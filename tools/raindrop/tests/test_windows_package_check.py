#!/usr/bin/env python3
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.
"""Offline tests for the Windows packaged-resource checker and workflow."""

import io
import json
import tempfile
import unittest
import zipfile
from pathlib import Path

from tools.raindrop.check_windows_package import check_package

REPO_ROOT = Path(__file__).resolve().parents[3]
WORKFLOW = REPO_ROOT / ".github/workflows/windows-package-check.yml"
MOZCONFIG = REPO_ROOT / "build/raindrop-windows-artifact.mozconfig"
CHECKER = REPO_ROOT / "tools/raindrop/check_windows_package.py"
BRANDING = REPO_ROOT / "browser/branding/raindrop"
BRANDING_PREFS_SOURCE = BRANDING / "pref/firefox-branding.js"
APP_ID = "{d9d2bf3e-372c-49a3-b630-2b67ba5b196e}"

APPLICATION_INI = """[App]
Vendor={vendor}
Name={name}
RemotingName={remoting}
Version=160.0.1
ID={app_id}
"""

BRANDING_PREFS = """pref("app.update.enabled", false);
pref("app.update.url", "");
pref("toolkit.telemetry.enabled", false);
pref("datareporting.healthreport.uploadEnabled", false);
pref("extensions.systemAddon.update.enabled", false);
pref("extensions.systemAddon.update.url", "");
"""

VERSION_INFO = {
    "installer": {
        "ProductName": "{brand}",
        "ProductVersion": "160.0.1",
        "FileDescription": "{brand} Installer",
        "CompanyName": "{brand}",
        "LegalCopyright": "Copyright (c) raindrop",
        "OriginalFilename": "setup.exe",
    },
    "helper": {
        "ProductName": "{brand}",
        "ProductVersion": "160.0.1",
        "FileDescription": "{brand} Helper",
        "CompanyName": "{brand}",
        "LegalCopyright": "Copyright (c) raindrop",
        "OriginalFilename": "helper.exe",
    },
}

STAGED_FILES = (
    "branding.nsi",
    "firefox64.ico",
    "wizHeader.bmp",
    "wizHeaderRTL.bmp",
    "wizWatermark.bmp",
)


class WindowsPackageCheckTests(unittest.TestCase):
    def make_tree(
        self,
        *,
        app_name="raindrop",
        package_name="raindrop-160.0.1.en-US.win64.zip",
        include_manifest=True,
        include_executable=True,
        include_upstream_font=False,
        staged_icon=None,
        version_brand="raindrop",
        branding_prefs=None,
    ):
        if branding_prefs is None:
            branding_prefs = BRANDING_PREFS_SOURCE.read_bytes()
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        root = Path(temp.name)
        objdir = root / "obj-raindrop-win-artifact"
        dist = objdir / "dist"
        instgen = objdir / "browser/installer/windows/instgen"
        dist.mkdir(parents=True)
        instgen.mkdir(parents=True)

        package_members = {
            "application.ini": APPLICATION_INI.format(
                vendor=app_name, name=app_name, remoting=app_name, app_id=APP_ID
            ).encode(),
            "browser/VisualElements/VisualElements_150.png": (
                BRANDING / "VisualElements_150.png"
            ).read_bytes(),
            "browser/VisualElements/VisualElements_70.png": (
                BRANDING / "VisualElements_70.png"
            ).read_bytes(),
            "browser/VisualElements/PrivateBrowsing_150.png": (
                BRANDING / "PrivateBrowsing_150.png"
            ).read_bytes(),
            "browser/VisualElements/PrivateBrowsing_70.png": (
                BRANDING / "PrivateBrowsing_70.png"
            ).read_bytes(),
            "private_browsing.exe": b"private executable",
            "uninstall/helper.exe": b"helper",
        }
        if include_executable:
            package_members["raindrop.exe"] = b"executable"
        if include_upstream_font:
            package_members["fonts/TwemojiMozilla.ttf"] = b"font"
        if include_manifest:
            package_members["firefox.VisualElementsManifest.xml"] = (
                BRANDING / "firefox.VisualElementsManifest.xml"
            ).read_bytes()
            package_members["private_browsing.VisualElementsManifest.xml"] = (
                BRANDING / "private_browsing.VisualElementsManifest.xml"
            ).read_bytes()

        omni = io.BytesIO()
        with zipfile.ZipFile(omni, "w") as archive:
            archive.writestr("defaults/preferences/firefox-branding.js", branding_prefs)
        package_members["browser/omni.ja"] = omni.getvalue()

        with zipfile.ZipFile(dist / package_name, "w") as archive:
            for name, data in package_members.items():
                archive.writestr(f"raindrop/{name}", data)

        (dist / "package_name.txt").write_text(package_name)
        (dist / "raindrop-160.0.1.en-US.win64.installer.exe").write_bytes(b"installer")
        for name in STAGED_FILES:
            data = (BRANDING / name).read_bytes()
            if name == "firefox64.ico" and staged_icon is not None:
                data = staged_icon
            (instgen / name).write_bytes(data)
        (instgen / "setup.exe").write_bytes(b"setup")

        version_info = json.loads(
            json.dumps(VERSION_INFO).replace("{brand}", version_brand)
        )
        version_path = dist / "installer-version-info.json"
        version_path.write_text(json.dumps(version_info))
        return objdir, version_path

    def test_valid_packaged_tree_has_no_failures(self):
        objdir, version_path = self.make_tree()
        self.assertEqual(check_package(objdir, version_path, REPO_ROOT), [])

    def test_artifact_build_tolerates_upstream_binary_paths(self):
        objdir, version_path = self.make_tree(
            include_executable=False, include_upstream_font=True
        )
        errors = check_package(objdir, version_path, REPO_ROOT, artifact_build=True)
        self.assertEqual(errors, [])

    def test_full_build_still_requires_raindrop_executable(self):
        objdir, version_path = self.make_tree(include_executable=False)
        errors = check_package(objdir, version_path, REPO_ROOT)
        self.assertTrue(any("raindrop.exe" in error for error in errors))

    def test_full_build_rejects_upstream_font_mark(self):
        objdir, version_path = self.make_tree(include_upstream_font=True)
        errors = check_package(objdir, version_path, REPO_ROOT)
        self.assertTrue(any("TwemojiMozilla.ttf" in error for error in errors))

    def test_wrong_application_identity_is_rejected(self):
        objdir, version_path = self.make_tree(app_name="Firefox")
        errors = check_package(objdir, version_path, REPO_ROOT)
        self.assertTrue(any("application.ini Name" in error for error in errors))

    def test_branded_package_name_is_required(self):
        objdir, version_path = self.make_tree(
            package_name="firefox-160.0.1.en-US.win64.zip"
        )
        errors = check_package(objdir, version_path, REPO_ROOT)
        self.assertTrue(
            any("Windows package name contains" in error for error in errors)
        )

    def test_missing_manifest_is_rejected(self):
        objdir, version_path = self.make_tree(include_manifest=False)
        errors = check_package(objdir, version_path, REPO_ROOT)
        self.assertTrue(
            any("firefox.VisualElementsManifest.xml" in error for error in errors)
        )

    def test_staged_icon_must_match_raindrop_artwork(self):
        objdir, version_path = self.make_tree(staged_icon=b"not-raindrop")
        errors = check_package(objdir, version_path, REPO_ROOT)
        self.assertTrue(any("firefox64.ico" in error for error in errors))

    def test_version_info_must_not_contain_marks(self):
        objdir, version_path = self.make_tree(version_brand="Mozilla Firefox")
        errors = check_package(objdir, version_path, REPO_ROOT)
        self.assertTrue(any("version info contains" in error for error in errors))

    def test_license_comments_do_not_fail_pref_mark_scan(self):
        objdir, version_path = self.make_tree()
        self.assertEqual(check_package(objdir, version_path, REPO_ROOT), [])

    def test_functional_pref_mark_is_rejected(self):
        prefs = (
            BRANDING_PREFS + '\npref("raindrop.links.update", "https://mozilla.org");\n'
        )
        objdir, version_path = self.make_tree(branding_prefs=prefs.encode())
        errors = check_package(objdir, version_path, REPO_ROOT)
        self.assertTrue(any("branding prefs contain" in error for error in errors))


class WindowsWorkflowWiringTests(unittest.TestCase):
    def test_workflow_runs_on_hosted_windows_for_pull_requests(self):
        text = WORKFLOW.read_text()
        self.assertIn("runs-on: windows-latest", text)
        self.assertIn("  pull_request:", text)
        self.assertIn("workflow_dispatch:", text)

    def test_mozillabuild_is_pinned_by_sha256(self):
        text = WORKFLOW.read_text()
        self.assertIn("MozillaBuildSetup-4.2.1.exe", text)
        self.assertIn(
            "8ef5b3d2087e4867dc75bf19f5e62f12e917f518239415723cc70896b6f1e3d8",
            text,
        )

    def test_artifact_revision_comes_from_sourcestamp(self):
        text = WORKFLOW.read_text()
        self.assertIn("sourcestamp.txt", text)
        self.assertIn("MOZ_ARTIFACT_REVISION", text)

    def test_workflow_invokes_checker(self):
        text = WORKFLOW.read_text()
        self.assertIn("python tools/raindrop/check_windows_package.py", text)
        self.assertIn("--artifact-build", text)
        self.assertTrue(CHECKER.is_file())

    def test_workflow_checks_raindrop_installer_metadata(self):
        text = WORKFLOW.read_text()
        self.assertIn("browser\\installer\\windows\\instgen\\setup.exe", text)
        self.assertIn(
            "obj-raindrop-win-artifact/browser/installer/windows/instgen/defines.nsi",
            text,
        )

    def test_artifact_mozconfig_selects_raindrop_branding(self):
        text = MOZCONFIG.read_text()
        self.assertIn("--with-branding=browser/branding/raindrop", text)
        self.assertIn("--enable-artifact-builds", text)
        self.assertIn("--disable-bootstrap", text)

    def test_workflow_installs_pinned_mozmake(self):
        text = WORKFLOW.read_text()
        self.assertIn("mozmake.tar.zst", text)
        self.assertIn(
            "8a7dbefcfdb35cc7e96acc8c10fc91064ad1cadb9a808e9df8b1a708e9aeafe0",
            text,
        )
        self.assertIn(".mozbuild\\mozmake", text)

    def test_workflow_points_configure_at_pinned_tools(self):
        text = WORKFLOW.read_text()
        self.assertIn("$env:GMAKE", text)
        self.assertIn("$env:NODEJS", text)
        self.assertIn("$env:MAKENSISU", text)
        self.assertIn("${env:7Z}", text)

    def test_workflow_installs_pinned_7z(self):
        text = WORKFLOW.read_text()
        self.assertIn("7zz.tar.zst", text)
        self.assertIn(
            "b34ecc2cc20babe4ebc18b54f53843ebc19797f8eca53ff5cb79dc33136e2563",
            text,
        )
        self.assertIn(".mozbuild\\7zz", text)

    def test_workflow_installs_mozilla_signed_nsis(self):
        text = WORKFLOW.read_text()
        self.assertIn("nsis-3.07-signed.zip", text)
        self.assertIn(
            "5856cd49b52870caa41814d71ce30c4304d5001b80c04f4eb7028ffedd91df7e",
            text,
        )
        self.assertIn(".mozbuild\\nsis", text)

    def test_workflow_installs_pinned_node(self):
        text = WORKFLOW.read_text()
        self.assertIn("node-v24.21.0-win-x64.zip", text)
        self.assertIn(
            "158f7685b44de51f6c0df1d153526cbcd3e1bc739a8dfc607721cef75de9e541",
            text,
        )
        self.assertIn(".mozbuild\\node", text)


if __name__ == "__main__":
    unittest.main()
