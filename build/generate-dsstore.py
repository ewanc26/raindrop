#!/usr/bin/env python3
"""Generate browser/branding/raindrop/dsstore from scratch.

The upstream branding directories ship a .DS_Store captured from a Finder
window; copying it would carry Mozilla's "Nightly" and "firefox-installer.dmg"
strings into a raindrop package. This builds a clean Finder layout instead:
icon view with the app icon on the right and the DMG background image enabled.

Run with: uv run --with ds_store python build/generate-dsstore.py
"""

from pathlib import Path

import ds_store
import ds_store.store

ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / "browser" / "branding" / "raindrop" / "dsstore"

bwsp = {
    "ShowStatusBar": False,
    "ShowToolbar": False,
    "ShowTabView": False,
    "ContainerShowSidebar": False,
    "WindowBounds": "{{200, 895}, {680, 425}}",
    "ShowSidebar": False,
}

# Finder writes the DMG background path into an alias blob. Building that alias
# here would bake a build-machine path into the repository, so leave it unset:
# the packager still copies background.png into .background/ and Finder falls
# back to the standard icon-view appearance.
icvp = {
    "backgroundColorBlue": 1.0,
    "iconSize": 128.0,
    "arrangeBy": "none",
    "textSize": 12.0,
    "backgroundColorRed": 1.0,
    "backgroundType": 2,
    "backgroundColorGreen": 1.0,
    "gridOffsetX": 0.0,
    "gridOffsetY": 0.0,
    "showItemInfo": False,
    "viewOptionsVersion": 1,
    "labelOnBottom": True,
    "showIconPreview": True,
    "gridSpacing": 100.0,
}

plist = ds_store.store.PlistCodec()
iloc = ds_store.store.ILocCodec()

entries = [
    ds_store.DSStoreEntry(".", "bwsp", plist, bwsp),
    ds_store.DSStoreEntry(".", "icvp", plist, icvp),
    ds_store.DSStoreEntry(".", "vSrn", "long", 1),
    ds_store.DSStoreEntry(" ", "Iloc", iloc, (472, 190)),
    ds_store.DSStoreEntry("raindrop.app", "Iloc", iloc, (472, 190)),
]

# Pass a w+b file object: ds_store's string-path "w" mode opens write-only,
# which its Buddy allocator then cannot read back.
with OUTPUT.open("w+b") as handle:
    with ds_store.DSStore.open(handle, "w", initial_entries=entries) as store:
        store.flush()

print(f"Wrote {OUTPUT}")
