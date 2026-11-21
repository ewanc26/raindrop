#!/usr/bin/env python3
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.
"""Verify the packaged Windows branding resources for issue #11.

The source-level branding checks cannot prove that a Windows build actually
ships the raindrop resources. This checker runs after a raindrop-branded
artifact build on a Windows runner and inspects the produced ZIP, the NSIS
staging directory and the PE version resources captured from the generated
installer/uninstaller.
"""

import argparse
import json
import re
import sys
import zipfile
from pathlib import Path

APP_ID = "{d9d2bf3e-372c-49a3-b630-2b67ba5b196e}"
BRANDING_DIR = Path("browser/branding/raindrop")

# Package paths are matched by suffix because the ZIP has a top-level
# app-named directory. Some filenames are fixed by the upstream build system;
# their contents must still be raindrop's.
PACKAGE_MEMBERS = {
    "application": "application.ini",
    "branding_prefs": "defaults/preferences/firefox-branding.js",
    "manifest": "firefox.VisualElementsManifest.xml",
    "private_manifest": "private_browsing.VisualElementsManifest.xml",
    "tile_150": "browser/VisualElements/VisualElements_150.png",
    "tile_70": "browser/VisualElements/VisualElements_70.png",
    "private_tile_150": "browser/VisualElements/PrivateBrowsing_150.png",
    "private_tile_70": "browser/VisualElements/PrivateBrowsing_70.png",
    "executable": "raindrop.exe",
    "private_executable": "private_browsing.exe",
    "uninstaller": "uninstall/helper.exe",
}

SOURCE_COPIES = {
    "manifest": "firefox.VisualElementsManifest.xml",
    "private_manifest": "private_browsing.VisualElementsManifest.xml",
    "tile_150": "VisualElements_150.png",
    "tile_70": "VisualElements_70.png",
    "private_tile_150": "PrivateBrowsing_150.png",
    "private_tile_70": "PrivateBrowsing_70.png",
}

STAGED_BRANDING_FILES = (
    "branding.nsi",
    "firefox64.ico",
    "wizHeader.bmp",
    "wizHeaderRTL.bmp",
    "wizWatermark.bmp",
)

REQUIRED_PREFS = {
    "app.update.enabled": "false",
    "app.update.url": '""',
    "toolkit.telemetry.enabled": "false",
    "datareporting.healthreport.uploadEnabled": "false",
    "extensions.systemAddon.update.enabled": "false",
    "extensions.systemAddon.update.url": '""',
}

PREF_RE = re.compile(r'pref\(\s*"([^"]+)"\s*,\s*([^;]+?)\s*\);')
FORBIDDEN_MARKS = ("mozilla", "firefox")
ALLOWED_PACKAGE_MARK_PATHS = (
    "defaults/preferences/firefox-branding.js",
    "firefox.VisualElementsManifest.xml",
)
ALLOWED_ARTIFACT_MARK_PATHS = ALLOWED_PACKAGE_MARK_PATHS + ("fonts/TwemojiMozilla.ttf",)


def _has_forbidden_mark(text):
    lowered = text.lower()
    return any(mark in lowered for mark in FORBIDDEN_MARKS)


def _find_member(names, suffix):
    matches = [name for name in names if name.replace("\\", "/").endswith(suffix)]
    if len(matches) != 1:
        return None
    return matches[0]


def _setting(text, key):
    match = re.search(rf"^\s*{re.escape(key)}\s*=\s*(.*?)\s*$", text, re.M | re.I)
    return match.group(1).strip() if match else None


def _version_value(values, key):
    for name, value in values.items():
        if name.lower() == key.lower():
            return str(value)
    return ""


def _check_version_info(label, values, errors):
    if not isinstance(values, dict):
        errors.append(f"{label}: version info is not an object")
        return

    values_text = " ".join(str(value) for value in values.values())
    if _has_forbidden_mark(values_text):
        errors.append(f"{label}: version info contains a Mozilla/Firefox mark")
    if not any("raindrop" in str(value).lower() for value in values.values()):
        errors.append(f"{label}: version info does not identify raindrop")

    for key in ("ProductName", "FileDescription", "CompanyName"):
        if not _version_value(values, key):
            errors.append(f"{label}: missing {key} in version info")


def check_package(objdir, version_info_path, source_root, artifact_build=False):
    """Return all packaged-resource failures; an empty list means success."""
    errors = []
    dist = objdir / "dist"

    package_name_path = dist / "package_name.txt"
    if not package_name_path.is_file():
        return ["dist/package_name.txt is missing"]
    package_name = package_name_path.read_text(encoding="utf-8").strip()
    if "raindrop" not in package_name.lower() or ".win64.zip" not in package_name:
        errors.append(f"unexpected Windows package name: {package_name!r}")
    if _has_forbidden_mark(package_name):
        errors.append(
            f"Windows package name contains a Mozilla/Firefox mark: {package_name!r}"
        )

    package_path = dist / package_name
    if not package_path.is_file():
        return errors + [f"packaged ZIP is missing: {package_path}"]

    required_members = dict(PACKAGE_MEMBERS)
    if artifact_build:
        required_members.pop("executable", None)

    allowed_mark_paths = (
        ALLOWED_ARTIFACT_MARK_PATHS if artifact_build else ALLOWED_PACKAGE_MARK_PATHS
    )

    members = {}
    try:
        with zipfile.ZipFile(package_path) as archive:
            names = archive.namelist()
            for key, suffix in required_members.items():
                member = _find_member(names, suffix)
                if member is None:
                    errors.append(f"packaged ZIP has no unique {suffix} entry")
                    continue
                members[key] = archive.read(member)

            for name in names:
                normalized = name.replace("\\", "/")
                if not _has_forbidden_mark(normalized):
                    continue
                if normalized.endswith(allowed_mark_paths):
                    continue
                errors.append(f"packaged path contains a Mozilla/Firefox mark: {name}")
    except zipfile.BadZipFile as exc:
        return errors + [f"cannot read packaged ZIP: {exc}"]

    if "application" in members:
        app_ini = members["application"].decode("utf-8", errors="replace")
        expected_settings = {
            "Name": "raindrop",
            "Vendor": "raindrop",
            "RemotingName": "raindrop",
            "ID": APP_ID,
        }
        for key, expected in expected_settings.items():
            actual = _setting(app_ini, key)
            if actual != expected:
                errors.append(
                    f"application.ini {key} is {actual!r}, expected {expected!r}"
                )
        if _has_forbidden_mark(app_ini):
            errors.append("application.ini contains a Mozilla/Firefox mark")

    if "branding_prefs" in members:
        prefs_text = members["branding_prefs"].decode("utf-8", errors="replace")
        prefs = {name: value.strip() for name, value in PREF_RE.findall(prefs_text)}
        for name, expected in REQUIRED_PREFS.items():
            if prefs.get(name) != expected:
                errors.append(f"packaged branding pref {name} is {prefs.get(name)!r}")
        if _has_forbidden_mark(prefs_text):
            errors.append("packaged branding prefs contain a Mozilla/Firefox mark")

    for key, source_name in SOURCE_COPIES.items():
        if key not in members:
            continue
        source = source_root / BRANDING_DIR / source_name
        if not source.is_file():
            errors.append(f"source branding file is missing: {source}")
        elif members[key] != source.read_bytes():
            errors.append(
                f"packaged {source_name} does not match raindrop's source asset"
            )

    instgen = objdir / "instgen"
    for name in STAGED_BRANDING_FILES:
        staged = instgen / name
        source = source_root / BRANDING_DIR / name
        if not staged.is_file():
            errors.append(f"NSIS staging directory is missing {name}")
        elif not source.is_file():
            errors.append(f"source branding file is missing: {source}")
        elif staged.read_bytes() != source.read_bytes():
            errors.append(
                f"staged installer resource {name} does not match raindrop's source asset"
            )

    if not (instgen / "setup.exe").is_file():
        errors.append("NSIS staging directory is missing setup.exe")

    installers = sorted(dist.glob("*.installer.exe"))
    if not installers:
        errors.append("no generated installer executable was found")
    elif _has_forbidden_mark(installers[0].name):
        errors.append(
            f"installer name contains a Mozilla/Firefox mark: {installers[0].name}"
        )

    version_info_path = Path(version_info_path)
    if not version_info_path.is_file():
        errors.append(f"version info JSON is missing: {version_info_path}")
    else:
        try:
            version_info = json.loads(version_info_path.read_text(encoding="utf-8"))
        except json.JSONDecodeError as exc:
            errors.append(f"cannot read version info JSON: {exc}")
        else:
            for label in ("installer", "helper"):
                if label not in version_info:
                    errors.append(f"version info JSON is missing {label}")
                else:
                    _check_version_info(label, version_info[label], errors)

    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--objdir", required=True, type=Path)
    parser.add_argument("--version-info", required=True, type=Path)
    parser.add_argument(
        "--source-root",
        type=Path,
        default=Path(__file__).resolve().parents[2],
    )
    parser.add_argument("--artifact-build", action="store_true")
    args = parser.parse_args()

    errors = check_package(
        args.objdir,
        args.version_info,
        args.source_root,
        artifact_build=args.artifact_build,
    )
    for error in errors:
        print(f"ERROR: {error}", file=sys.stderr)
    if errors:
        print(
            f"Windows package check failed with {len(errors)} problem(s)",
            file=sys.stderr,
        )
        return 1
    print("Windows packaged resources are raindrop-branded")
    return 0


if __name__ == "__main__":
    sys.exit(main())
