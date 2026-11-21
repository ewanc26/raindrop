#!/usr/bin/env python3
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
"""Offline tests for raindrop's bundled marks and first-run bookmarks.

The macOS bundle signature and the default bookmark set come from upstream
files that a rebase can silently restore to Mozilla values. These checks keep
the issue #11 branding contract in source so that cannot happen unnoticed.
"""

import re
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]

RELEASE_MOZCONFIG = REPO_ROOT / "build/raindrop.mozconfig"
STOCK_MOZCONFIG = REPO_ROOT / "build/raindrop-unofficial.mozconfig"
MOZ_BUILD = REPO_ROOT / "browser/app/moz.build"
INFO_PLIST = REPO_ROOT / "browser/app/macbuild/Contents/Info.plist.in"
DEFAULT_BOOKMARKS = REPO_ROOT / "browser/base/content/default-bookmarks.html"
DEFAULT_BOOKMARKS_FTL = (
    REPO_ROOT / "browser/locales/en-US/browser/profile/default-bookmarks.ftl"
)
BROWSER_BOOKMARK_TEST = (
    REPO_ROOT
    / "browser/components/places/tests/browser/browser_bookmarks_checkDefaultBookmarks.js"
)
UNIT_BOOKMARK_TEST = (
    REPO_ROOT / "browser/components/places/tests/unit/test_browserGlue_prefs.js"
)
BRANDING_DIR = REPO_ROOT / "browser/branding/raindrop"
WINDOWS_NSI = BRANDING_DIR / "branding.nsi"
WINDOWS_MANIFESTS = (
    BRANDING_DIR / "firefox.VisualElementsManifest.xml",
    BRANDING_DIR / "private_browsing.VisualElementsManifest.xml",
)


def strip_license_comments(text):
    text = re.sub(r"<!--.*?-->", "", text, flags=re.S)
    return re.sub(r"^\s*(?:#|//|/\*).*?$", "", text, flags=re.M)


class BrandingSelectionTests(unittest.TestCase):
    def test_release_config_selects_raindrop_branding(self):
        self.assertIn(
            "--with-branding=browser/branding/raindrop",
            RELEASE_MOZCONFIG.read_text(),
        )

    def test_stock_config_selects_upstream_unofficial_branding(self):
        self.assertIn(
            "--with-branding=browser/branding/unofficial",
            STOCK_MOZCONFIG.read_text(),
        )


class BundleSignatureTests(unittest.TestCase):
    def test_macos_pkginfo_is_raindrop(self):
        text = MOZ_BUILD.read_text()
        self.assertIn('pkginfo="APPLRDRO"', text)
        self.assertNotIn("APPLMOZB", text)

    def test_macos_bundle_signature_is_raindrop(self):
        text = INFO_PLIST.read_text()
        self.assertIn("<key>CFBundleSignature</key>", text)
        self.assertIn("<string>RDRO</string>", text)
        self.assertNotIn("MOZB", text)


class DefaultBookmarksTests(unittest.TestCase):
    def setUp(self):
        self.text = DEFAULT_BOOKMARKS.read_text()

    def test_all_bookmark_targets_are_first_party(self):
        hrefs = re.findall(r'href="([^"]+)"', self.text)
        self.assertTrue(hrefs, "default bookmarks contain no links")
        for href in hrefs:
            with self.subTest(href=href):
                self.assertTrue(href.startswith("about:"), href)
        for target in ("about:home", "about:preferences", "about:about"):
            self.assertIn(target, hrefs)

    def test_no_mozilla_bookmark_folder_or_localization_ids(self):
        self.assertIn("<h3>raindrop</h3>", self.text)
        self.assertNotIn("data-l10n-id", self.text)
        lowered = self.text.lower()
        self.assertNotIn("mozilla firefox", lowered)
        self.assertNotIn("firefox nightly resources", lowered)

    def test_upstream_bookmark_l10n_file_is_removed(self):
        self.assertFalse(DEFAULT_BOOKMARKS_FTL.exists())


class WindowsResourceTests(unittest.TestCase):
    def test_installer_uses_raindrop_identity_and_blank_stub_urls(self):
        text = WINDOWS_NSI.read_text()
        self.assertIn('BrandFullName         "raindrop"', text)
        self.assertIn('CompanyName           "raindrop"', text)
        for define in (
            "URLStubDownloadX86",
            "URLStubDownloadAMD64",
            "URLStubDownloadAArch64",
            "URLManualDownload",
        ):
            with self.subTest(define=define):
                self.assertRegex(text, rf'!define {define}\s+""')

    def test_installer_has_no_mozilla_endpoints(self):
        text = strip_license_comments(WINDOWS_NSI.read_text())
        for endpoint in ("mozilla.org", "mozilla.com", "firefox.com"):
            with self.subTest(endpoint=endpoint):
                self.assertNotIn(endpoint, text.lower())

    def test_tile_manifests_have_no_mozilla_marks(self):
        for manifest in WINDOWS_MANIFESTS:
            with self.subTest(manifest=manifest.name):
                text = strip_license_comments(manifest.read_text())
                lowered = text.lower()
                self.assertNotIn("firefox", lowered)
                self.assertNotIn("mozilla", lowered)


class BookmarkTestExpectationsTests(unittest.TestCase):
    def test_browser_test_expects_first_party_bookmarks(self):
        text = BROWSER_BOOKMARK_TEST.read_text()
        for target in ("about:home", "about:preferences", "about:about"):
            with self.subTest(target=target):
                self.assertIn(f'"{target}"', text)
        self.assertNotIn("support.mozilla.org", text)
        self.assertNotIn("blog.nightly.mozilla.org", text)

    def test_unit_test_expects_raindrop_folder(self):
        text = UNIT_BOOKMARK_TEST.read_text()
        self.assertIn('bm.title, "raindrop"', text)
        self.assertNotIn('"Mozilla Firefox"', text)
        self.assertNotIn('"Firefox Nightly Resources"', text)


if __name__ == "__main__":
    unittest.main()
