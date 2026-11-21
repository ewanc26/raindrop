#!/usr/bin/env python3
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
"""Offline tests for the AT Protocol setup verifier."""

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import verify_atproto_setup  # noqa: E402

REPO_ROOT = Path(__file__).resolve().parents[3]


def write_valid_tree(root):
    for check in verify_atproto_setup.CHECKS:
        path = root / check.relative_path
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text("\n".join(check.required) + "\n")


class VerifyAtprotoSetupTests(unittest.TestCase):
    def test_repository_passes(self):
        self.assertEqual(verify_atproto_setup.verify_tree(REPO_ROOT), [])

    def test_generated_tree_passes(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_valid_tree(root)
            self.assertEqual(verify_atproto_setup.verify_tree(root), [])

    def test_missing_file_is_reported(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_valid_tree(root)
            (root / "netwerk/protocol/atproto/components.conf").unlink()
            failures = verify_atproto_setup.verify_tree(root)
            self.assertTrue(
                any("missing netwerk/protocol/atproto/components.conf" in f for f in failures)
            )

    def test_missing_needle_is_reported(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_valid_tree(root)
            path = root / "modules/libpref/init/all.js"
            path.write_text(path.read_text().replace("network.atprotocol.enabled", "removed"))
            failures = verify_atproto_setup.verify_tree(root)
            self.assertTrue(
                any("all.js missing 'pref(\"network.atprotocol.enabled\", true);'" in f for f in failures)
            )

    def test_forbidden_needle_is_reported(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_valid_tree(root)
            path = root / "browser/base/content/default-bookmarks.html"
            path.write_text(path.read_text() + "\nat://ewancroft.uk\n")
            failures = verify_atproto_setup.verify_tree(root)
            self.assertTrue(any("contains forbidden 'at://'" in f for f in failures))


if __name__ == "__main__":
    unittest.main()
