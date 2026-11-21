#!/usr/bin/env python3
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
"""Offline tests for harness compatibility with raindrop's rebranded app name."""

import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
BRANDING_CONFIGURE = REPO_ROOT / "browser/branding/raindrop/configure.sh"
RUNNERS = REPO_ROOT / "testing/mozbase/mozrunner/mozrunner/runners.py"
XPCSHELL = REPO_ROOT / "testing/xpcshell/runxpcshelltests.py"


class RebrandedNameTests(unittest.TestCase):
    def test_basename_remains_raindrop(self):
        text = BRANDING_CONFIGURE.read_text()
        self.assertIn("MOZ_APP_BASENAME=raindrop", text)
        self.assertNotIn("MOZ_APP_NAME=firefox", text)


class RunnerAliasTests(unittest.TestCase):
    def test_raindrop_uses_firefox_runner(self):
        text = RUNNERS.read_text()
        self.assertRegex(text, r'"raindrop"\s*:\s*FirefoxRunner\s*,')
        self.assertNotIn('"raindrop": Runner', text)


class XpcshellAppDirTests(unittest.TestCase):
    def setUp(self):
        self.text = XPCSHELL.read_text()

    def test_falls_back_to_firefox_appdir(self):
        self.assertIn('"firefox-appdir" in self.test_object', self.text)
        self.assertIn('app_dir_key = "firefox-appdir"', self.text)

    def test_fallback_precedes_appdir_selection(self):
        fallback = self.text.index('app_dir_key = "firefox-appdir"')
        selection = self.text.index("if app_dir_key and app_dir_key in self.test_object:")
        self.assertLess(fallback, selection)


if __name__ == "__main__":
    unittest.main()
