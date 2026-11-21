# raindrop branding

`browser/branding/raindrop/` is a drop-in Firefox branding directory. It is
selected with:

```sh
export MOZCONFIG="$PWD/build/raindrop.mozconfig"
```

`build/raindrop-unofficial.mozconfig` remains the default for local developer
builds: with no `--with-branding` option Firefox falls back to
`browser/branding/unofficial`, which is intentional (see
[TRADEMARKS.md](TRADEMARKS.md)).

## Artwork provenance

Every icon, word mark and installer image in the directory is generated from
original SVG artwork in `browser/branding/raindrop/source/`. The mark is a
single raindrop: the README's image of a drop travelling directly from a cloud
to its destination, with no gateway in between. It is not a recoloured Firefox
logo, and no Bluesky/AT Protocol marks are used.

Some filenames are fixed by Firefox's build system (for example
`firefox.icns`, `firefox-wordmark.svg`, `pref/firefox-branding.js`). They are
*paths*, not branding: the contents are raindrop artwork and strings.

| Source | Generated assets |
| --- | --- |
| `source/icon.svg` | app/desktop icons, ICO/ICNS, `Assets.car`, installer tiles, MSIX assets, DMG background |
| `source/icon-private.svg` | private-browsing icons and tiles |
| `source/glyph.svg`, `source/glyph-private.svg` | transparent about-screen logos |
| `source/wordmark.svg` | about-screen word marks |
| `source/document.svg`, `source/document-pdf.svg` | file-type icons |

## Regenerating assets

On macOS:

```sh
brew install librsvg imagemagick
bash build/generate-branding-assets.sh
uv run --with ds_store python build/generate-dsstore.py
```

`iconutil` and `actool` come from Xcode. The DS_Store is generated rather than
copied from an upstream branding directory: the upstream file contains
"Nightly" and `firefox-installer.dmg` strings that would survive inside a
raindrop DMG.

## Product names and endpoints

- `configure.sh` sets `MOZ_APP_DISPLAYNAME`, `MOZ_APP_BASENAME` and
  `MOZ_APP_REMOTINGNAME` to `raindrop`, plus the mac bundle id suffix `browser`.
  The basename controls the executable name, profile directory and system
  integration hooks, so nothing falls back to `firefox`.
- `browser/moz.configure` replaces the implied `MOZ_APP_VENDOR` and `MOZ_APP_ID`
  with raindrop values. These are the only identity settings that cannot live in
  a branding directory's `configure.sh`: the options accept implied values only.
  `MOZ_APP_VENDOR=raindrop` and the application UUID
  `{d9d2bf3e-372c-49a3-b630-2b67ba5b196e}` now appear in `application.ini` and
  `about:` pages instead of Mozilla/Firefox values.
- `locales/en-US/brand.ftl` and `brand.properties` set every brand string to
  raindrop; `trademarkInfo` carries the non-affiliation disclaimer.
- `pref/firefox-branding.js` disables update checks and clears Mozilla update
  URLs, disables telemetry upload, and switches off background services that
  would otherwise transmit to Mozilla without user action (Normandy remote
  config, Contile sponsored tiles, MITM-detection priming, and the MARS new-tab
  ads endpoint at `ads.mozilla.org`). An offline test in
  `tools/raindrop/tests/test_branding_prefs.py` locks these in.
- `branding.nsi` contains no Mozilla company name, support URL or download
  endpoint. Stub-installer download URLs are blank until raindrop hosts its own
  artifacts.
- `build/raindrop.mozconfig` passes `--with-distribution-id=uk.ewancroft.raindrop`
  so the macOS bundle identifier does not inherit `org.mozilla`.

## Status

Done at source level: branding directory, original artwork, generated assets,
brand strings, update/telemetry prefs, mozconfig wiring, branded bundle id.

Still to verify (tracked in
[issue #11](https://github.com/ewanc26/raindrop/issues/11)):

- [ ] `./mach build` with `build/raindrop.mozconfig` succeeds.
- [ ] `./mach package` produces binaries/installers with no Mozilla mark,
      including the DMG `.DS_Store` and Windows resources.
- [ ] No Firefox-branded chrome or defaults survive outside this directory
      (a follow-up patch queue item once a build can be inspected).
- [ ] `docs/TRADEMARKS.md` and `NOTICE` re-checked against Mozilla's current
      guidelines before a first public release.
