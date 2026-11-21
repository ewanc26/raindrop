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
- [ ] Windows packaged-resource branding is inspected on a Windows runner.
      The `Windows package check` workflow builds a raindrop-branded artifact
      package on `windows-latest` and verifies the packaged ZIP, NSIS-staged
      installer resources, uninstaller and PE version information.

The last two gates are infrastructure-only and are tracked in issues #10 and
#11. They do not require further code changes.

## Pre-tag verification

Run with the tracked release mozconfig exported in the same shell:

```sh
export MOZCONFIG="$PWD/build/raindrop.mozconfig"
./mach build
python3.12 ./mach gtest 'AtURI.*:DidResolver.*:PdsClient.*:HandleResolver.*:RecordView.*:CidVerifier.*:ErrorView.*:PrivacyFlags.*:LexiconView.*:AtRecordResolver.*'
./mach xpcshell-test netwerk/protocol/atproto/tests/test_at_protocol.js
./mach xpcshell-test browser/components/places/tests/unit/test_browserGlue_prefs.js
./mach mochitest browser/components/places/tests/browser/browser_bookmark_current_tabs.js --headless
./mach mochitest browser/components/places/tests/browser/browser_bookmarks_checkDefaultBookmarks.js --headless
./mach mochitest browser/components/preferences/tests/browser_at_protocol_pane.js --headless
python3.12 -m unittest discover -s tools/raindrop/tests
python3.12 ./mach package
```

The expected counts are 74 GTests across 10 suites, 29 atproto xpcshell checks,
22 browser-pref xpcshell checks and 66 offline tests. The second xpcshell test
catches `MOZ_APP_NAME` and `appdir` harness mismatches introduced by rebranding;
the mochitests catch app-ID gates that stop browser actors from starting, and
the preferences mochitest checks the AT Protocol settings pane and its prefs. The
offline suite also guards the app-ID gates for automation, downloads and reader
mode that a rebase can silently restore.
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
bitmap resources, and the installer/uninstaller version resources. A green run
and the uploaded package artifacts close the Windows half of issue #11.

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
