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
- [ ] The stock-upstream build passes in CI on a registered self-hosted runner
      (`RAINDROP_BUILD_RUNNER` and `RAINDROP_UPSTREAM_TAG` set). The pristine
      build is proven locally; CI proof is the outstanding part of issue #10.
- [ ] Windows packaged-resource branding is inspected on a Windows runner.
      Installer, manifest and icon sources are already checked statically.

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
python3.12 -m unittest discover -s tools/raindrop/tests
python3.12 ./mach package
```

The expected counts are 73 GTests across 10 suites, 27 atproto xpcshell checks,
22 browser-pref xpcshell checks and 44 offline tests. The second xpcshell test
catches `MOZ_APP_NAME` and `appdir` harness mismatches introduced by rebranding;
the mochitests catch app-ID gates that stop browser actors from starting. The
offline suite also guards the app-ID gates for automation, downloads and reader
mode that a rebase can silently restore.
GTest suites must be passed as one colon-separated argument; separate
invocations can report a false green. Confirm `./mach format` and
`git diff --check` are clean.

Inspect the produced DMG before tagging: the app bundle must carry raindrop's
vendor, name and remoting name, disabled update/telemetry prefs, the `RDRO`
bundle signature, and no Mozilla marks in default bookmarks or shipped
resources.

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
