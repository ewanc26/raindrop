#!/usr/bin/env python3
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
"""Detect when a newer signed Firefox release is due for a raindrop rebase.

This implements the detection half of ADR 0002. It has no third-party
dependencies so it can run in CI with a bare ``actions/setup-python``. The
network fetch is isolated behind :func:`fetch_product_details`; everything
else is pure and unit-testable with a fixture.

Exit status is 0 whenever the check *completes* (including when an upgrade is
due), so a scheduled job can read the machine-readable output. Pass
``--fail-on-upgrade`` to get status 10 (import due) or 11 (dot release needs
triage) for interactive use.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
import urllib.request
from datetime import date, timedelta
from pathlib import Path
from typing import Any

PRODUCT_DETAILS_URL = (
    "https://product-details.mozilla.org/1.0/firefox_versions.json"
)
LATEST_KEY = "LATEST_FIREFOX_VERSION"
RELEASE_DATE_KEY = "LAST_RELEASE_DATE"

UP_TO_DATE = 0
IMPORT_DUE = 10
TRIAGE_DUE = 11
ERROR = 2

_VERSION_RE = re.compile(r"^(\d+)\.(\d+)(?:\.(\d+))?")
_TRACKED_RE = re.compile(r"Upstream version\s*\|\s*Firefox\s+([0-9]+(?:\.[0-9]+)+)")


class UpstreamError(Exception):
    """Raised for malformed input or a missing tracked version."""


def parse_version(value: str) -> tuple[int, int, int]:
    """Parse a Firefox release version into ``(major, minor, patch)``.

    Suffixes used on non-release channels (``esr``, ``b``, ``a``) are ignored;
    the release-channel check only ever sees clean ``N.N`` / ``N.N.N`` values.
    """
    match = _VERSION_RE.match(value.strip())
    if not match:
        raise UpstreamError(f"Not a Firefox version: {value!r}")
    major, minor = int(match.group(1)), int(match.group(2))
    patch = int(match.group(3)) if match.group(3) else 0
    return (major, minor, patch)


def format_version(parts: tuple[int, int, int]) -> str:
    major, minor, patch = parts
    return f"{major}.{minor}.{patch}" if patch else f"{major}.{minor}"


def classify(tracked: str, latest: str) -> str:
    """Return ``none``, ``dot`` or ``release`` for the version bump.

    Firefox ships a new train (major version) every two weeks and never ships
    a ``160.1``, so the only distinction that matters for ADR 0002 is a patch
    bump within the current train (``dot``) versus a new train (``release``).
    Dot releases must be triaged — imported only for a security fix or a fix
    to code raindrop ships — while a new train is the scheduled rebase.
    """
    tracked_parts = parse_version(tracked)
    latest_parts = parse_version(latest)
    if latest_parts <= tracked_parts:
        return "none"
    if latest_parts[:2] == tracked_parts[:2]:
        return "dot"
    return "release"


def action_for(kind: str) -> str:
    return {"none": "none", "dot": "triage", "release": "import"}[kind]


def working_deadline(release_day: date, working_days: int = 5) -> date:
    """Return the date ``working_days`` weekdays after ``release_day``.

    ADR 0002 asks for a rebase within five working days of a release.
    """
    day = release_day
    remaining = working_days
    while remaining:
        day += timedelta(days=1)
        if day.weekday() < 5:  # Monday == 0
            remaining -= 1
    return day


def read_tracked_version(repo_root: Path) -> str:
    """Read the pinned upstream version from ``docs/BUILDING.md``.

    BUILDING.md is the documented record of the exact upstream version in
    ADR 0002 point 5, and unlike git tags it is present in a shallow CI clone.
    """
    building = repo_root / "docs" / "BUILDING.md"
    if not building.is_file():
        raise UpstreamError(f"Missing {building}")
    match = _TRACKED_RE.search(building.read_text(encoding="utf-8"))
    if not match:
        raise UpstreamError(f"No tracked Firefox version in {building}")
    return match.group(1)


def fetch_product_details(source: str) -> dict[str, Any]:
    """Load product-details JSON from a URL or a local fixture path."""
    if source.startswith(("http://", "https://")):
        with urllib.request.urlopen(source, timeout=30) as response:
            return json.load(response)
    return json.loads(Path(source).read_text(encoding="utf-8"))


def check(
    tracked: str, details: dict[str, Any], today: date | None = None
) -> dict[str, Any]:
    """Build the machine-readable upgrade decision for a product-details payload."""
    latest = details.get(LATEST_KEY)
    if not latest:
        raise UpstreamError(f"Missing {LATEST_KEY} in product details")
    kind = classify(tracked, latest)
    today = today or date.today()
    result: dict[str, Any] = {
        "tracked_version": tracked,
        "latest_version": latest,
        "upgrade_due": kind != "none",
        "release_kind": kind,
        "action": action_for(kind),
        "release_date": details.get(RELEASE_DATE_KEY, ""),
    }
    release_date = result["release_date"]
    try:
        parsed = date.fromisoformat(release_date)
        result["rebase_deadline"] = working_deadline(parsed).isoformat()
        result["days_remaining"] = (working_deadline(parsed) - today).days
    except (ValueError, TypeError):
        result["rebase_deadline"] = ""
        result["days_remaining"] = None
    return result


def tarball_urls(version: str) -> tuple[str, str]:
    """Return the signed release tarball URL and its detached-signature URL."""
    base = (
        "https://archive.mozilla.org/pub/firefox/releases/"
        f"{version}/source/firefox-{version}.source.tar.xz"
    )
    return base, base + ".asc"


def import_plan(version: str, sha256: str) -> list[str]:
    """Return the exact shell commands for a signed ADR 0002 import + rebase.

    Kept as data so it can be unit-tested and printed with ``--plan`` without
    touching a working tree. Commands are intentionally explicit: signature
    verification must succeed before the tarball is trusted, and the tag
    annotation records version, URL and digest.
    """
    tarball, signature = tarball_urls(version)
    return [
        f"curl -fsSLO {tarball}",
        f"curl -fsSLO {signature}",
        "curl -fsSL https://packages.mozilla.org/rpm/firefox/signing-key.gpg -o mozilla-signing-key.gpg",
        "# Verify the imported key fingerprints against Mozilla's published values before proceeding.",
        "gpg --dearmor < mozilla-signing-key.gpg > mozilla-keyring.gpg",
        f"gpgv --keyring ./mozilla-keyring.gpg firefox-{version}.source.tar.xz.asc firefox-{version}.source.tar.xz",
        f"shasum -a 256 firefox-{version}.source.tar.xz  # expect {sha256}",
        f"git switch upstream && git rm -rf . && tar -xJf firefox-{version}.source.tar.xz --strip-components=1 && git add -A",
        f'git commit -m "Import Firefox {version} release source"',
        (
            f'git tag -a firefox-{version}'
            f' -m "Firefox {version} signed release source"'
            f' -m "Tarball: {tarball}"'
            f' -m "SHA-256: {sha256}"'
        ),
        "git switch main && git rebase firefox-" + version,
        "# Resolve any conflicts, rebuild, then run atproto GTests and xpcshell before pushing.",
    ]


def _emit(result: dict[str, Any], as_json: bool) -> None:
    if as_json:
        print(json.dumps(result, sort_keys=True))
        return
    for key, value in result.items():
        print(f"{key}={value}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", default=PRODUCT_DETAILS_URL,
                        help="product-details URL or local JSON fixture path")
    parser.add_argument("--repo", default=".", type=Path,
                        help="repository root containing docs/BUILDING.md")
    parser.add_argument("--tracked-version",
                        help="override the version read from BUILDING.md")
    parser.add_argument("--today", help="ISO date to use for deadline math (tests)")
    parser.add_argument("--json", action="store_true", help="emit a JSON object")
    parser.add_argument("--fail-on-upgrade", action="store_true",
                        help="exit 10/11 when an upgrade is due")
    parser.add_argument("--plan", metavar="VERSION",
                        help="print the signed import + rebase command plan and exit")
    parser.add_argument("--sha256", default="<recorded sha256>",
                        help="expected digest for --plan annotation")
    args = parser.parse_args(argv)

    try:
        if args.plan:
            for command in import_plan(args.plan, args.sha256):
                print(command)
            return 0
        tracked = args.tracked_version or read_tracked_version(args.repo)
        details = fetch_product_details(args.source)
        today = date.fromisoformat(args.today) if args.today else None
        result = check(tracked, details, today)
    except (UpstreamError, OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return ERROR

    _emit(result, args.json)
    if args.fail_on_upgrade:
        return {"none": UP_TO_DATE, "dot": TRIAGE_DUE}.get(
            result["release_kind"], IMPORT_DUE
        )
    return UP_TO_DATE


if __name__ == "__main__":
    sys.exit(main())
