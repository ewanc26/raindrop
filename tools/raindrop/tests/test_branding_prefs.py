#!/usr/bin/env python3
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
"""Offline tests for raindrop's no-phone-home branding preferences.

The raindrop branding directory claims it keeps the build from calling home as
Firefox. These tests lock that contract into source so a rebase cannot silently
re-enable a background service that transmits to Mozilla.
"""

import re
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
BRANDING_PREFS = (
    REPO_ROOT / "browser/branding/raindrop/pref/firefox-branding.js"
)

PREF_RE = re.compile(r'pref\(\s*"([^"]+)"\s*,\s*([^;]+?)\s*\);')

# Preferences that must be set by raindrop branding so the build does not
# contact Mozilla background services on its own.
REQUIRED_PREFS = {
    "app.update.enabled": "false",
    "app.update.url": '""',
    "toolkit.telemetry.enabled": "false",
    "datareporting.healthreport.uploadEnabled": "false",
    # Services that poll or push to Mozilla without user action.
    "app.normandy.enabled": "false",
    "browser.topsites.contile.enabled": "false",
    "security.certerrors.mitm.priming.enabled": "false",
    # The add-on manager's background check would fetch a system-addon list
    # from aus5.mozilla.org, which raindrop does not consume.
    "extensions.systemAddon.update.enabled": "false",
    "extensions.systemAddon.update.url": '""',
    # New-tab ads service (MARS) reaches ads.mozilla.org on a normal new tab.
    "browser.newtabpage.activity-stream.unifiedAds.spocs.enabled": "false",
    "browser.newtabpage.activity-stream.unifiedAds.tiles.enabled": "false",
    "browser.newtabpage.activity-stream.unifiedAds.endpoint": '""',
}


def parse_prefs(text):
    return {name: value.strip() for name, value in PREF_RE.findall(text)}


class BrandingPrefsTests(unittest.TestCase):
    def setUp(self):
        self.prefs = parse_prefs(BRANDING_PREFS.read_text())

    def test_no_phone_home_prefs_are_set(self):
        for name, expected in REQUIRED_PREFS.items():
            with self.subTest(pref=name):
                self.assertIn(name, self.prefs, f"{name} is not set by branding")
                self.assertEqual(self.prefs[name], expected)


if __name__ == "__main__":
    unittest.main()
