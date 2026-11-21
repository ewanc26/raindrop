# raindrop branding

`browser/branding/raindrop/` is a drop-in Firefox branding directory. It is
selected with:

```sh
export MOZCONFIG="$PWD/build/raindrop.mozconfig"
```

`build/raindrop-unofficial.mozconfig` is for stock-source bootstrap builds: it
selects upstream's `browser/branding/unofficial` explicitly, and with no
`--with-branding` option `browser/confvars.sh` falls back to the same directory.
Upstream's `unofficial` branding is a local-development fallback and still
contains Nightly/Firefox/Mozilla brand strings; it is not for distributing
raindrop. Release builds use `build/raindrop.mozconfig`.

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
  integration hooks, so nothing falls back to `firefox`. Test harnesses that
  key off `MOZ_APP_NAME` have explicit raindrop aliases: `mozrunner` selects
  `FirefoxRunner`, and xpcshell falls back to `firefox-appdir` when an upstream
  manifest does not declare `<appname>-appdir`. The raindrop application UUID
  is also listed on the `app-startup` `nsBrowserGlue` category in
  `browser/components/BrowserComponents.manifest`; without it browser actors
  never register and chrome startup hangs.

  The same UUID is accepted wherever upstream restricts desktop behavior to
  Firefox: `remote/shared/AppInfo.sys.mjs` for automation,
  `toolkit/components/downloads/DownloadIntegration.sys.mjs` for
  blocked-download recovery, and `toolkit/components/reader/ReaderMode.sys.mjs`
  for local language detection. The Marionette client maps it to `fxdesktop`.
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
  config, Contile sponsored tiles, MITM-detection priming, the add-on manager's
  system-addon check at `aus5.mozilla.org`, and the MARS new-tab ads endpoint at
  `ads.mozilla.org`). An offline test in
  `tools/raindrop/tests/test_branding_prefs.py` locks these in.
- `branding.nsi` contains no Mozilla company name, support URL or download
  endpoint. Stub-installer download URLs are blank until raindrop hosts its own
  artifacts.
- `tools/raindrop/tests/test_branding_marks.py` locks in the macOS bundle
  signature and first-party default bookmarks so a rebase cannot restore
  upstream Mozilla marks unnoticed.
- `build/raindrop.mozconfig` passes `--with-distribution-id=uk.ewancroft.raindrop`
  so the macOS bundle identifier does not inherit `org.mozilla`.

## Status

Done at source level: branding directory, original artwork, generated assets,
brand strings, update/telemetry prefs, mozconfig wiring, branded bundle id.

Still to verify (tracked in
[issue #11](https://github.com/ewanc26/raindrop/issues/11)):

- [x] `./mach build` with `build/raindrop.mozconfig` succeeds (`BUILD_EXIT=0`).
- [x] `./mach package` produces binaries/installers with no Mozilla mark. The
      macOS DMG builds (`PACKAGE_EXIT=0`) and its `.DS_Store` is byte-identical to
      raindrop's clean artwork. `CFBundleIconFile=firefox.icns` is a fixed
      build-system path whose contents are byte-identical to raindrop's own
      artwork, so it is not a mark. The updater/helper code-signing requirements
      no longer carry Mozilla's Apple team ID: the official mozconfig enables
      `--enable-mac-elevated-updates-with-generic-certs` on macOS, so the
      packaged `SMAuthorizedClients`/`SMPrivilegedExecutables` strings use
      `anchor apple generic` only. The macOS bundle signature is now raindrop's
      own `CFBundleSignature=RDRO` (`PkgInfo` `APPLRDRO`); no `MOZB` remains in
      the shipped app bundle. Windows resources still need a Windows build
      runner for packaged inspection.
- [x] No Firefox-branded chrome or defaults survive outside this directory.
      First-run default bookmarks now ship a single `raindrop` folder with
      first-party `about:home`, `about:preferences`, and `about:about` targets.
      The upstream Mozilla links, icons, and l10n IDs were removed from
      `browser/base/content/default-bookmarks.html` and its two tests. The now
      unused `browser/locales/en-US/browser/profile/default-bookmarks.ftl` and
      its fluent-lint exclusion were removed, so no Firefox default strings
      ship in `omni.ja`. The shipped archive was inspected to confirm the new
      content.
- [ ] `docs/TRADEMARKS.md` and `NOTICE` re-checked against Mozilla's current
      guidelines before a first public release.
