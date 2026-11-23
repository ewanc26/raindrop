#!/usr/bin/env python3
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
"""Offline tests for raindrop's no-phone-home branding preferences.

The raindrop branding directory claims it keeps the build from calling home as
Firefox. These tests lock that contract into source so a rebase cannot silently
re-enable a background service that transmits to Mozilla.
"""

import json
import re
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
BRANDING_PREFS = REPO_ROOT / "browser/branding/raindrop/pref/firefox-branding.js"
THEME_MANIFEST = REPO_ROOT / "browser/themes/addons/raindrop/manifest.json"
RAINDROP_THEME_ID = "raindrop@ewancroft.uk"
RAINDROP_THEME_PREF = "extensions.activeThemeID"

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

    def test_raindrop_theme_is_default(self):
        self.assertEqual(
            self.prefs.get(RAINDROP_THEME_PREF),
            f'"{RAINDROP_THEME_ID}"',
            "raindrop's theme is not the default for new profiles",
        )

    def test_theme_manifest_matches_default(self):
        manifest_text = THEME_MANIFEST.read_text()
        manifest = json.loads(manifest_text)
        self.assertEqual(
            manifest["browser_specific_settings"]["gecko"]["id"],
            RAINDROP_THEME_ID,
        )
        self.assertEqual(manifest["name"], "raindrop")
        self.assertIn("theme", manifest)
        self.assertIn("dark_theme", manifest)
        for mark in ("Mozilla", "Firefox", "Bluesky"):
            self.assertNotIn(mark, manifest_text)

    def test_theme_text_contrast_meets_wcag_aa(self):
        manifest = json.loads(THEME_MANIFEST.read_text())
        pairs = (
            ("toolbar_text", "toolbar"),
            ("tab_text", "frame"),
            ("toolbar_field_text", "toolbar_field"),
            ("bookmark_text", "toolbar"),
            ("popup_text", "popup"),
            ("sidebar_text", "sidebar"),
            ("sidebar_highlight_text", "sidebar_highlight"),
        )
        for variant in ("theme", "dark_theme"):
            colors = manifest[variant]["colors"]
            for foreground, background in pairs:
                with self.subTest(variant=variant, pair=foreground):
                    self.assertGreaterEqual(
                        contrast_ratio(colors[foreground], colors[background]),
                        4.5,
                        f"{variant} {foreground} does not meet WCAG AA",
                    )


def relative_luminance(value):
    value = value.lstrip("#")
    channels = [int(value[i : i + 2], 16) / 255 for i in (0, 2, 4)]
    linear = [
        channel / 12.92 if channel <= 0.04045 else ((channel + 0.055) / 1.055) ** 2.4
        for channel in channels
    ]
    return 0.2126 * linear[0] + 0.7152 * linear[1] + 0.0722 * linear[2]


def contrast_ratio(foreground, background):
    lighter = max(relative_luminance(foreground), relative_luminance(background))
    darker = min(relative_luminance(foreground), relative_luminance(background))
    return (lighter + 0.05) / (darker + 0.05)


if __name__ == "__main__":
    unittest.main()
