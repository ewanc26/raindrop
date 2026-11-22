# Releasing raindrop

This file defines the gates and the exact procedure for the first tagged
release. Tag naming and upstream tracking follow
[ADR 0002](decisions/0002-upstream-tracking.md).

## Release gates

A tag is cut only when every gate below is satisfied:

- [x] The signed upstream release is imported and tagged `firefox-<version>`
      with its SHA-256 recorded in [BUILDING.md](BUILDING.md).
- [x] The raindrop patch queue is rebased onto that tag and landed on `main`.
- [x] The design review is signed off and recorded in
      [ADR 0005](decisions/0005-identity-authority-and-freshness-signoff.md).
- [x] Trademark sign-off is recorded in [TRADEMARKS.md](TRADEMARKS.md); the
      release uses no Mozilla branding marks outside required license notices
      and the non-affiliation disclaimer, and makes no unprompted Mozilla
      requests.
- [x] macOS packaged branding, bundle signature and no-phone-home prefs are
      verified in the shipped app bundle.
- [x] The stock-upstream build passes in CI on a registered self-hosted runner
      (`RAINDROP_BUILD_RUNNER` and `RAINDROP_UPSTREAM_TAG` set). Dispatch run
      `44912582850` succeeded, closing the CI half of issue #10.
- [x] Windows packaged-resource branding is inspected on a Windows runner.
      The `Windows package check` workflow builds a raindrop-branded artifact
      package on `windows-latest` and verifies the packaged ZIP, NSIS-staged
      installer resources, uninstaller and PE version information. Dispatch run
      `44921258664` succeeded, closing the Windows half of issue #11.

Both infrastructure-only gates are now closed (issues #10 and #11).

## Pre-tag verification

Run with the tracked release mozconfig exported in the same shell:

```sh
export MOZCONFIG="$PWD/build/raindrop.mozconfig"
./mach build
python3.12 tools/raindrop/verify_atproto_setup.py
python3.12 ./mach gtest 'AtDocumentPolicy.*:AtURI.*:DidResolver.*:PdsClient.*:HandleResolver.*:RecordView.*:CidVerifier.*:ErrorView.*:PrivacyFlags.*:LexiconView.*:AtRecordResolver.*:RepoView.*:CollectionView.*'
./mach xpcshell-test netwerk/protocol/atproto/tests/test_at_protocol.js
./mach xpcshell-test browser/components/places/tests/unit/test_browserGlue_prefs.js
./mach mochitest browser/components/places/tests/browser/browser_bookmark_current_tabs.js --headless
./mach mochitest browser/components/places/tests/browser/browser_bookmarks_checkDefaultBookmarks.js --headless
./mach mochitest browser/components/preferences/tests/browser_at_protocol_pane.js --headless
./mach mochitest netwerk/protocol/atproto/tests/browser/browser_at_protocol_navigation.js --headless
python3.12 -m unittest discover -s tools/raindrop/tests
python3.12 ./mach package
```

The expected counts are 145 GTests across 13 suites, 56 atproto xpcshell
subtests, 21 browser-pref xpcshell subtests and 71 offline tests. The second
xpcshell test catches `MOZ_APP_NAME` and `appdir` harness mismatches introduced
by rebranding; the mochitests catch app-ID gates that stop browser actors from
starting, and the preferences mochitest checks the AT Protocol settings pane and
its prefs. The offline suite also guards the app-ID gates for automation,
downloads and reader mode that a rebase can silently restore, and runs on every
pull request through `.github/workflows/offline-suite.yml`.
GTest suites must be passed as one colon-separated argument; separate
invocations can report a false green. Confirm `./mach format` and
`git diff --check` are clean.

Inspect the produced DMG before tagging: the app bundle must carry raindrop's
vendor, name and remoting name, disabled update/telemetry prefs, the `RDRO`
bundle signature, and no Mozilla marks in default bookmarks or shipped
resources.

## Windows packaged-resource gate

Run the `Windows package check` workflow on `windows-latest` (manually or on a
pull request). It installs SHA-256-pinned MozillaBuild 4.2.1, derives
`MOZ_ARTIFACT_REVISION` from the release revision in `sourcestamp.txt`, then
builds and packages with `build/raindrop-windows-artifact.mozconfig`. An
artifact build reuses upstream compiled C++ binaries, so this gate proves the
raindrop resources, preferences, installer assets and PE version metadata —
not the at-protocol C++ implementation, which is covered by the macOS build
and tests above.

The workflow finishes with:

```sh
python3.12 tools/raindrop/check_windows_package.py \
  --objdir obj-raindrop-win-artifact \
  --version-info obj-raindrop-win-artifact/dist/installer-version-info.json
```

That checker confirms the ZIP name and its `application.ini`, packed branding
prefs, Visual Elements manifests and tile artwork, the NSIS-staged icon and
bitmap resources, and the installer/uninstaller version resources. Dispatch run
`44921258664` was green at `930c87a39a` and uploaded the `windows-package`
artifact, closing the Windows half of issue #11.

## Tag naming and provenance

A fork release is an annotated tag:

```
v<raindrop-version>-fx<firefox-version>
```

For example, `v0.1.0-fx160.0.1` is the first raindrop release on Firefox
160.0.1. The tag annotation must name the upstream tag (`firefox-160.0.1`),
the tarball SHA-256 from [BUILDING.md](BUILDING.md), and the verification
results from the pre-tag run. Fork tags are created only after the queue has
landed on `main`.

The upstream tag stays pristine: raindrop commits are never added to the
`upstream` branch, and `main` is rebased, never merged, onto each new upstream
tag.

## Publishing

Attach the verified DMG and its SHA-256 to the GitHub release for the tag.
Record the release date and any deviation from the gates above here. If a
future release has to proceed while a gate is open, the annotation and the
release notes must say which gate was waived and why.

### v0.1.0-fx160.0.1

Released 2026-11-21 from `main` at `16ec25c7e8`. All gates were green; no
deviations or waivers. The GitHub release
(`https://github.com/ewanc26/raindrop/releases/tag/v0.1.0-fx160.0.1`) carries
`raindrop-160.0.1.en-US.mac.dmg` and its `.sha256` sidecar. DMG SHA-256:
`1dc03fa148ddc0616bbc367340ca286e03f11c51a2fe82cb697dce8bd6c59ad7`.

### v0.2.0-fx160.0.1

Released 2026-11-21 from `main` at `a62e6031e2`. The GitHub release
(`https://github.com/ewanc26/raindrop/releases/tag/v0.2.0-fx160.0.1`) carries
`raindrop-160.0.1.en-US.mac.dmg` and its `.sha256` sidecar. DMG SHA-256:
`7da878cb26dc40575326df9ab48607d0efa29705e770fc24f7fb4427e31efa99`.

Pre-tag verification: build exit 0 with 0 compiler warnings, 113/113 GTests,
45 atproto and 22 browser-pref xpcshell checks, three mochitests PASS, 66 offline
tests OK, `./mach package` exit 0, `./mach format` and `git diff --check` clean.
The packaged DMG was inspected for raindrop branding and prefs. Hosted
`unittest`, `package` and Windows package checks were green on the merged PR.

Deviation: the stock-upstream `stock-build` check was skipped because no
self-hosted runner is registered (`RAINDROP_BUILD_RUNNER` is unset). The signed
upstream revision and its earlier green build proof (run `44912582850`) are
unchanged; no source from upstream was modified for this release. This is
recorded in the tag annotation and release notes rather than silently waived.

### v0.3.0-fx160.0.1

Released 2026-11-21 from `main` at `d164309f2d`. The GitHub release
(`https://github.com/ewanc26/raindrop/releases/tag/v0.3.0-fx160.0.1`) carries
`raindrop-160.0.1.en-US.mac.dmg` and its `.sha256` sidecar. DMG SHA-256:
`18048991951147bdffffad2c862c32fa84e138b8c20e794250c156865573d8a3`.

Pre-tag verification: build exit 0 with 0 compiler warnings, 117/117 GTests,
45 atproto and 22 browser-pref xpcshell checks, three mochitests PASS (AT
Protocol pane 20 checks), 66 offline tests OK, `./mach package` exit 0,
`./mach format` and `git diff --check` clean. The packaged DMG was inspected for
raindrop branding and prefs, including the new `showImages` pref. Hosted
`unittest`, `package` and Windows package checks were green on the merged PR.
The same `stock-build` deviation as v0.2.0 applies and is recorded in the tag
annotation and release notes.

### v0.4.0-fx160.0.1

Released 2026-11-21 from `main` at `1b0d9a5cfb`. The GitHub release
(`https://github.com/ewanc26/raindrop/releases/tag/v0.4.0-fx160.0.1`) carries
`raindrop-160.0.1.en-US.mac.dmg` and its `.sha256` sidecar. DMG SHA-256:
`f3484ac5834c46eff2022b5631e66e6840201925b0990d5c304421ca209df965`.

Pre-tag verification: build exit 0 with 0 compiler warnings, 122/122 GTests,
45 atproto and 22 browser-pref xpcshell checks, three mochitests PASS (AT
Protocol pane 20 checks), 66 offline tests OK, `./mach package` exit 0,
`./mach format` and `git diff --check` clean. The packaged DMG was inspected for
raindrop branding and prefs. Hosted `unittest`, `package` and Windows package
checks were green on the merged PR. The same `stock-build` deviation as v0.2.0
applies and is recorded in the tag annotation and release notes.

### v0.5.0-fx160.0.1

Released 2026-11-21 from `main` at `289b6b43f8`. The GitHub release
(`https://github.com/ewanc26/raindrop/releases/tag/v0.5.0-fx160.0.1`) carries
`raindrop-160.0.1.en-US.mac.dmg` and its `.sha256` sidecar. DMG SHA-256:
`2af02586b496b3bc247e9b2b06b88436b78fd7e761490dbfddde3acfe18a1256`.

Pre-tag verification: build exit 0 with 0 compiler warnings, 125/125 GTests,
45 atproto and 22 browser-pref xpcshell checks, three mochitests PASS (AT
Protocol pane 20 checks), 66 offline tests OK, `./mach package` exit 0,
`./mach format` and `git diff --check` clean. The packaged DMG was inspected for
raindrop branding and prefs. Hosted `unittest` and `package` checks were green
on the merged PR. `stock-build` was skipped because `RAINDROP_BUILD_RUNNER` is
unset, and `submit-gradle` failed with the known non-blocking infrastructure
issue. Both deviations are recorded in the tag annotation and release notes.

### v0.6.0-fx160.0.1

Released 2026-11-21 from `main` at `594fe6f4ee`. The GitHub release
(`https://github.com/ewanc26/raindrop/releases/tag/v0.6.0-fx160.0.1`) carries
`raindrop-160.0.1.en-US.mac.dmg` and its `.sha256` sidecar. DMG SHA-256:
`bcd6eddc33a5d2bbf1d39ca9df6d0077fdb869ce0d553316a8401b5863c96a8f`.

Pre-tag verification: build exit 0 with 0 compiler warnings, setup verifier OK,
125/125 GTests, 45 atproto and 22 browser-pref xpcshell checks, mochitests PASS,
71 offline tests OK, `./mach package` exit 0, `./mach format` 0 problems and
`git diff --check` clean. Workflow YAML parsed. The packaged DMG was inspected
for raindrop branding. Hosted `Offline suite` and `Windows package check` were
green on the merged PR. `stock-build` was skipped because
`RAINDROP_BUILD_RUNNER` is unset, and `submit-gradle` failed with the known
non-blocking infrastructure issue. Both deviations are recorded in the tag
annotation and release notes.

### v0.7.0-fx160.0.1

Released 2026-11-21 from `main` at `b7eaa6029d`. The GitHub release
(`https://github.com/ewanc26/raindrop/releases/tag/v0.7.0-fx160.0.1`) carries
`raindrop-160.0.1.en-US.mac.dmg` and its `.sha256` sidecar. DMG SHA-256:
`717906be1356ea82bd84bfb4e2c67428c8902dbaefd381f6173c919b054ed66d`.

Pre-tag verification: build exit 0 with 0 compiler warnings, setup verifier OK,
128 GTests across 13 suites, 49 atproto and 22 browser-pref xpcshell checks,
four mochitests with 37 subtests PASS, 71 offline tests OK, `./mach package`
exit 0, `./mach format` 0 problems and `git diff --check` clean. Hosted
`unittest` and `package` checks were green on the merged PR. `stock-build` was
skipped because `RAINDROP_BUILD_RUNNER` is unset, and `submit-gradle` failed
with the known non-blocking infrastructure issue. Both deviations are recorded
in the tag annotation and release notes.

### v0.8.0-fx160.0.1

Released 2026-11-21 from `main` at `337fc1f0c2`. The GitHub release
(`https://github.com/ewanc26/raindrop/releases/tag/v0.8.0-fx160.0.1`) carries
`raindrop-160.0.1.en-US.mac.dmg` and its `.sha256` sidecar. DMG SHA-256:
`4b31c25789bdb614dd98fbbd84c46aa0708a31c0538d6d2dcbfcd1462264a79d`.

Pre-tag verification: build exit 0 with 0 compiler warnings, setup verifier OK,
129 GTests across 13 suites, 50 atproto and 21 browser-pref xpcshell subtests,
four mochitests with 35 passes, 71 offline tests OK, `./mach package` exit 0,
`./mach format` 0 problems and `git diff --check` clean. The packaged
`greprefs.js` and inline-image preference description were inspected. Hosted
`Offline suite` and `Windows package check` were green on the merged PR.
`stock-build` was skipped because `RAINDROP_BUILD_RUNNER` is unset, and
`submit-gradle` failed with the known non-blocking infrastructure issue. Both
deviations are recorded in the tag annotation and release notes.

### v0.9.0-fx160.0.1

Released 2026-11-22 from `main` at `ef59deb8e6`. The GitHub release
(`https://github.com/ewanc26/raindrop/releases/tag/v0.9.0-fx160.0.1`) carries
`raindrop-160.0.1.en-US.mac.dmg` and its `.sha256` sidecar. DMG SHA-256:
`3e3fef29c919ac0c3f406cbf9d4b1389a4da57d90347e4ba92ef82411acd01cc`.

Pre-tag verification: build exit 0 with 0 compiler warnings, setup verifier OK,
136 GTests across 13 suites, 56 atproto and 21 browser-pref xpcshell subtests,
four mochitests with 82 passes and 2 todo, 71 offline tests OK, `./mach package`
exit 0, `./mach format` 0 problems and `git diff --check` clean. The packaged
`greprefs.js` and the six new preference descriptions were inspected. Hosted
`Offline suite` and `Windows package check` were green on the merged PR.
`stock-build` was skipped because `RAINDROP_BUILD_RUNNER` is unset, and
`submit-gradle` failed with the known non-blocking infrastructure issue. Both
deviations are recorded in the tag annotation and release notes.

### v0.10.0-fx160.0.1

Released 2026-11-22 from `main` at `1dc680fa57`. The GitHub release
(`https://github.com/ewanc26/raindrop/releases/tag/v0.10.0-fx160.0.1`) carries
`raindrop-160.0.1.en-US.mac.dmg` and its `.sha256` sidecar. DMG SHA-256:
`b1f8e420aad600f5fc4254167d3485e04e93af372180f9bdf2f5b49965cbf010`.

Pre-tag verification: build exit 0 with 0 compiler warnings, setup verifier OK,
142 GTests across 13 suites, 56 atproto and 21 browser-pref xpcshell subtests,
four mochitests with 82 passes and 2 todo, 71 offline tests OK, `./mach package`
exit 0, `./mach format` 0 problems and `git diff --check` clean. The signed
upstream tarball and release key were verified. Hosted `Offline suite`,
`package` and `Windows package check` were green on the merged PR.
`stock-build` was skipped because `RAINDROP_BUILD_RUNNER` is unset, and
`submit-gradle` failed with the known non-blocking `config.status.json not
found` infrastructure issue. Both deviations are recorded in the tag annotation
and release notes.
