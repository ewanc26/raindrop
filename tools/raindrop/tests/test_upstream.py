#!/usr/bin/env python3
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
"""Offline tests for the ADR 0002 upgrade-detection tooling."""

import json
import subprocess
import sys
import tempfile
import unittest
from datetime import date
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import upstream  # noqa: E402

FIXTURE = Path(__file__).parent / "fixtures" / "product_details.json"


class ParseVersionTests(unittest.TestCase):
    def test_two_and_three_component_versions(self):
        self.assertEqual(upstream.parse_version("160.0"), (160, 0, 0))
        self.assertEqual(upstream.parse_version("160.0.1"), (160, 0, 1))

    def test_suffixes_are_ignored(self):
        self.assertEqual(upstream.parse_version("153.7.0esr"), (153, 7, 0))
        self.assertEqual(upstream.parse_version("163.0a1"), (163, 0, 0))

    def test_garbage_is_rejected(self):
        with self.assertRaises(upstream.UpstreamError):
            upstream.parse_version("not-a-version")

    def test_round_trip(self):
        for text in ("160.0", "160.0.1"):
            self.assertEqual(upstream.format_version(upstream.parse_version(text)), text)


class ClassifyTests(unittest.TestCase):
    def test_dot_release(self):
        self.assertEqual(upstream.classify("160.0", "160.0.1"), "dot")

    def test_new_train_is_a_release(self):
        self.assertEqual(upstream.classify("160.0", "161.0"), "release")
        self.assertEqual(upstream.classify("160.0", "163.0"), "release")

    def test_no_bump(self):
        self.assertEqual(upstream.classify("160.0", "160.0"), "none")
        self.assertEqual(upstream.classify("160.0.1", "160.0"), "none")

    def test_actions(self):
        self.assertEqual(upstream.action_for("dot"), "triage")
        self.assertEqual(upstream.action_for("release"), "import")
        self.assertEqual(upstream.action_for("none"), "none")


class DeadlineTests(unittest.TestCase):
    def test_five_working_days_skips_weekend(self):
        # 2026-11-20 is a Friday; +5 working days = Friday 2026-11-27.
        self.assertEqual(upstream.working_deadline(date(2026, 11, 20)).isoformat(),
                         "2026-11-27")

    def test_midweek(self):
        # 2026-11-23 is a Monday; +5 working days = Monday 2026-11-30.
        self.assertEqual(upstream.working_deadline(date(2026, 11, 23)).isoformat(),
                         "2026-11-30")


class CheckTests(unittest.TestCase):
    def test_dot_release_needs_triage_with_deadline(self):
        details = json.loads(FIXTURE.read_text())
        result = upstream.check("160.0", details, today=date(2026, 11, 21))
        self.assertTrue(result["upgrade_due"])
        self.assertEqual(result["release_kind"], "dot")
        self.assertEqual(result["action"], "triage")
        self.assertEqual(result["rebase_deadline"], "2026-11-27")
        self.assertEqual(result["days_remaining"], 6)

    def test_up_to_date(self):
        details = json.loads(FIXTURE.read_text())
        result = upstream.check("160.0.1", details, today=date(2026, 11, 21))
        self.assertFalse(result["upgrade_due"])
        self.assertEqual(result["action"], "none")

    def test_missing_latest_key(self):
        with self.assertRaises(upstream.UpstreamError):
            upstream.check("160.0", {})


class TrackedVersionTests(unittest.TestCase):
    def test_reads_building_md(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            docs = root / "docs"
            docs.mkdir()
            (docs / "BUILDING.md").write_text(
                "| Item | Value |\n| --- | --- |\n| Upstream version | Firefox 160.0 |\n"
            )
            self.assertEqual(upstream.read_tracked_version(root), "160.0")

    def test_missing_file(self):
        with tempfile.TemporaryDirectory() as tmp, self.assertRaises(upstream.UpstreamError):
            upstream.read_tracked_version(Path(tmp))


class ImportPlanTests(unittest.TestCase):
    def test_urls_are_signed_release_artifacts(self):
        tarball, signature = upstream.tarball_urls("160.0.1")
        self.assertEqual(
            tarball,
            "https://archive.mozilla.org/pub/firefox/releases/160.0.1/source/firefox-160.0.1.source.tar.xz",
        )
        self.assertEqual(signature, tarball + ".asc")

    def test_plan_verifies_signature_before_import(self):
        plan = upstream.import_plan("160.0.1", "abc123")
        joined = "\n".join(plan)
        self.assertIn("gpgv --keyring", joined)
        self.assertIn("git rebase firefox-160.0.1", joined)
        self.assertIn("SHA-256: abc123", joined)
        self.assertLess(joined.index("gpgv --keyring"), joined.index("git switch upstream"))

    def test_plan_keyring_path_is_explicit(self):
        plan = "\n".join(upstream.import_plan("160.0.1", "abc123"))
        self.assertIn("gpgv --keyring ./mozilla-keyring.gpg", plan)
        self.assertNotIn("gpgv --keyring mozilla-keyring.gpg", plan)

    def test_tag_annotation_has_no_literal_escapes(self):
        tag = [c for c in upstream.import_plan("160.0.1", "abc123")
               if c.startswith("git tag")][0]
        self.assertNotIn("\\n", tag)
        self.assertEqual(tag.count("-m "), 3)


class CliTests(unittest.TestCase):
    def test_cli_checks_fixture_and_exits_zero(self):
        result = subprocess.run(
            [sys.executable, str(Path(upstream.__file__)), "--source", str(FIXTURE),
             "--tracked-version", "160.0", "--today", "2026-11-21", "--json"],
            capture_output=True, text=True,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        payload = json.loads(result.stdout)
        self.assertEqual(payload["latest_version"], "160.0.1")
        self.assertEqual(payload["action"], "triage")

    def test_fail_on_upgrade_uses_triage_code(self):
        result = subprocess.run(
            [sys.executable, str(Path(upstream.__file__)), "--source", str(FIXTURE),
             "--tracked-version", "160.0", "--fail-on-upgrade"],
            capture_output=True, text=True,
        )
        self.assertEqual(result.returncode, upstream.TRIAGE_DUE)

    def test_plan_mode_is_offline(self):
        result = subprocess.run(
            [sys.executable, str(Path(upstream.__file__)), "--plan", "161.0",
             "--sha256", "deadbeef"],
            capture_output=True, text=True,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("git rebase firefox-161.0", result.stdout)


if __name__ == "__main__":
    unittest.main()
