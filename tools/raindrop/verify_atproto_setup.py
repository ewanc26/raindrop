#!/usr/bin/env python3
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
"""Offline verification of raindrop's AT Protocol setup wiring."""

import argparse
import sys
from dataclasses import dataclass
from pathlib import Path

DEFAULT_ROOT = Path(__file__).resolve().parents[2]

GTEST_FILTER = (
    "AtDocumentPolicy.*:AtURI.*:DidResolver.*:PdsClient.*:HandleResolver.*:"
    "RecordView.*:"
    "CidVerifier.*:ErrorView.*:PrivacyFlags.*:LexiconView.*:"
    "AtRecordResolver.*:RepoView.*:CollectionView.*"
)


@dataclass(frozen=True)
class FileCheck:
    relative_path: str
    required: tuple[str, ...]
    forbidden: tuple[str, ...] = ()


CHECKS = (
    FileCheck(
        "netwerk/protocol/atproto/components.conf",
        (
            "@mozilla.org/network/protocol;1?name=at",
            "'scheme': 'at'",
            "URI_NORELATIVE",
            "URI_NOAUTH",
            "URI_LOADABLE_BY_ANYONE",
        ),
        (
            "URI_DANGEROUS_TO_LOAD",
            "URI_NON_PERSISTABLE",
            "URI_HAS_WEB_EXPOSED_ORIGIN",
        ),
    ),
    FileCheck(
        "modules/libpref/init/StaticPrefList.yaml",
        ("network.url.simple_uri_unknown_schemes", 'value: "at"'),
    ),
    FileCheck(
        "modules/libpref/init/all.js",
        (
            'pref("network.atprotocol.enabled", true);',
            'pref("network.atprotocol.record.showProvenance", true);',
            'pref("network.atprotocol.record.showRawJson", true);',
            'pref("network.atprotocol.record.showImages", false);',
            'pref("network.atprotocol.view.theme", "system");',
            'pref("network.atprotocol.view.textSize", "medium");',
            'pref("network.atprotocol.view.width", "standard");',
            'pref("network.atprotocol.view.accent", "blue");',
            'pref("network.atprotocol.view.density", "comfortable");',
            'pref("network.atprotocol.collection.pageSize", "50");',
        ),
    ),
    FileCheck(
        "browser/components/preferences/config/at-protocol.mjs",
        (
            "network.atprotocol.enabled",
            "network.atprotocol.record.showProvenance",
            "network.atprotocol.record.showRawJson",
            "network.atprotocol.record.showImages",
            "network.atprotocol.view.theme",
            "network.atprotocol.view.textSize",
            "network.atprotocol.view.width",
            "network.atprotocol.view.accent",
            "network.atprotocol.view.density",
            "network.atprotocol.collection.pageSize",
            'SettingGroupManager.registerGroup("atProtocol"',
        ),
    ),
    FileCheck(
        "browser/components/preferences/preferences.js",
        (
            "atProtocol: {",
            'module: "chrome://browser/content/preferences/config/at-protocol.mjs"',
        ),
    ),
    FileCheck(
        "browser/base/content/default-bookmarks.html",
        ("about:preferences#atProtocol",),
        ("at://",),
    ),
    FileCheck("netwerk/protocol/moz.build", ('"atproto"',)),
    FileCheck(
        "netwerk/protocol/atproto/AtDocumentPolicy.h",
        (
            "kAtDocumentCsp",
            "AppendDocumentSecurityMeta",
            "AppendDocumentStyle",
            "ViewStyleOptions",
            "LoadViewStyleOptions",
        ),
    ),
    FileCheck(
        "netwerk/protocol/atproto/LexiconView.cpp",
        (
            "app.bsky.richtext.facet#link",
            "app.bsky.richtext.facet#mention",
            "app.bsky.richtext.facet#tag",
            "IsSafeExternalLink",
            "ClampToUtf8Boundary",
            "noopener noreferrer",
        ),
    ),
    FileCheck(
        "netwerk/protocol/atproto/AtProtocolHandler.cpp",
        ("X-Content-Type-Options", "Referrer-Policy", "X-Frame-Options"),
    ),
    FileCheck(
        "netwerk/protocol/atproto/tests/browser/browser.toml",
        ("browser_at_protocol_navigation.js",),
    ),
    FileCheck(
        "netwerk/protocol/atproto/tests/TestAtURI.cpp", ("OfficialAtUriSyntaxVectors",)
    ),
    FileCheck(
        "netwerk/protocol/atproto/tests/TestLexiconView.cpp",
        ("RejectsUnsafeLinkFacets", "ClampsFacetsToUtf8Boundaries"),
    ),
    FileCheck(
        "netwerk/protocol/atproto/CollectionView.cpp",
        (
            "kMaxSummaryBytes",
            "SanitizeSummary",
            "SetRecordSummary",
            "AppendRecordSummary",
            "IsValidNsid",
        ),
    ),
    FileCheck(
        "netwerk/protocol/atproto/tests/TestCollectionView.cpp",
        ("EscapesAndCapsSummaries", "KeepsProvenanceLinksWhenPdsReturnsOtherDid"),
    ),
    FileCheck(
        "netwerk/protocol/atproto/tests/test_at_protocol.js",
        ("checkLoadURIWithPrincipal",),
    ),
    FileCheck(
        "browser/components/preferences/tests/browser_at_protocol_pane.js",
        ("network.atprotocol.enabled",),
    ),
    FileCheck(
        ".github/workflows/offline-suite.yml",
        ("verify_atproto_setup.py",),
    ),
    FileCheck(
        ".github/workflows/atproto-integration.yml",
        ("verify_atproto_setup.py", GTEST_FILTER),
    ),
    FileCheck(
        "docs/RELEASING.md",
        (GTEST_FILTER,),
    ),
    FileCheck(
        "docs/ATPROTO_SETUP.md",
        ("at://ewancroft.uk", "about:preferences#atProtocol"),
    ),
    FileCheck(
        "docs/BUILDING.md",
        ("ATPROTO_SETUP.md",),
    ),
    FileCheck(
        "README.md",
        ("ATPROTO_SETUP.md",),
    ),
)


def verify_tree(root: Path) -> list[str]:
    failures = []
    for check in CHECKS:
        path = root / check.relative_path
        try:
            text = path.read_text()
        except OSError:
            failures.append(f"missing {check.relative_path}")
            continue
        for needle in check.required:
            if needle not in text:
                failures.append(f"{check.relative_path} missing {needle!r}")
        for needle in check.forbidden:
            if needle in text:
                failures.append(f"{check.relative_path} contains forbidden {needle!r}")
    return failures


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=DEFAULT_ROOT)
    args = parser.parse_args(argv)

    failures = verify_tree(args.root)
    if failures:
        print("AT Protocol setup verification: FAILED")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print("AT Protocol setup verification: OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
