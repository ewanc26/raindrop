# Upgrading the Firefox base

raindrop tracks signed Firefox release source on a two-week cadence. ADR 0002
fixes the policy; this runbook is the mechanical procedure. Automation detects
and reports a due upgrade — the import itself stays a reviewed, signed manual
step so a bad tarball or a missing signature can never be merged silently.

## How an upgrade is detected

`.github/workflows/upgrade-check.yml` runs daily and on demand. It calls
`tools/raindrop/upstream.py`, which reads the pinned version from
`docs/BUILDING.md`, compares it to Mozilla's
[product-details endpoint](https://product-details.mozilla.org/1.0/firefox_versions.json),
and classifies the bump:

| Kind | Meaning | Action |
| --- | --- | --- |
| `none` | Already current | none |
| `dot` | Patch bump in the current train (`160.0` → `160.0.1`) | **triage**: import only for a security fix or a fix to shipped code |
| `release` | New two-week train (`160.0` → `161.0`) | import and rebase |

On `dot` or `release` the job opens or refreshes one issue titled
`Firefox <version> upgrade (<kind>) — <action>` with the five-working-day
deadline. Close that issue only once the import, rebase, docs and tag are done.

To run the check locally, without creating an issue:

```sh
python3 tools/raindrop/upstream.py              # human-readable key=value output
python3 tools/raindrop/upstream.py --json       # machine-readable
python3 tools/raindrop/upstream.py --fail-on-upgrade   # exit 11 = triage, 10 = import
```

The script has no third-party dependencies. Pass `--source path/to.json` to
check against a saved product-details payload. Its logic is covered by
`tools/raindrop/tests/test_upstream.py` (run with
`python3 -m unittest discover -s tools/raindrop/tests -v`).

## Importing a release

1. **Verify the source before trusting it.** Fetch the signed tarball and
   detached signature, and the Mozilla signing key. Check the key fingerprints
   against [Mozilla's published values](https://packages.mozilla.org/rpm/firefox/signing-key.gpg)
   before trusting it; signing subkeys rotate. The key in ADR 0002 is a record,
   not a trust anchor.

   ```sh
   V=<version>   # e.g. 161.0
   curl -fsSLO "https://archive.mozilla.org/pub/firefox/releases/$V/source/firefox-$V.source.tar.xz{,.asc}"
   curl -fsSL https://packages.mozilla.org/rpm/firefox/signing-key.gpg -o mozilla-signing-key.gpg
   gpg --show-keys --fingerprint mozilla-signing-key.gpg   # eyeball the fingerprints
   gpg --dearmor < mozilla-signing-key.gpg > mozilla-keyring.gpg
   gpgv --keyring mozilla-keyring.gpg "firefox-$V.source.tar.xz.asc" "firefox-$V.source.tar.xz"
   shasum -a 256 "firefox-$V.source.tar.xz"
   ```

   `gpgv` must report a Good signature from Mozilla Software Releases and the
   digest must be recorded. Run `python3 tools/raindrop/upstream.py --plan "$V"
   --sha256 <digest>` to print the exact import + rebase command sequence for
   review before executing any of it.

2. **Import onto `upstream`.** That branch is a linear, pristine sequence of
   release imports with no raindrop commits. Replace its tree with the verified
   source and commit one import, then tag it annotated with the upstream
   version, tarball URL and SHA-256:

   ```sh
   git switch upstream
   git rm -rf . && tar -xJf "firefox-$V.source.tar.xz" --strip-components=1 && git add -A
   git commit -m "Import Firefox $V release source"
   git tag -a "firefox-$V" -m "Firefox $V signed release source

   Tarball: https://archive.mozilla.org/pub/firefox/releases/$V/source/firefox-$V.source.tar.xz
   SHA-256: <digest>"
   ```

   The previous import tag stays; never rewrite `upstream` history.

3. **Rebase the patch queue.** `main` is rebased, never merged, onto the new
   import so each patch stays one logical commit and conflicts map back to one
   change.

   ```sh
   git switch main && git rebase "firefox-$V"
   ```

   Resolve any conflict in the thin integration points named in ADR 0001 (the
   `netwerk/protocol/atproto/` module should rarely conflict). Preserve commit
   boundaries — do not squash.

4. **Rebuild and test.** Use the recorded mozconfig and run the full atproto
   suite before pushing. See [docs/BUILDING.md](BUILDING.md) for the build
   setup.

   ```sh
   MOZCONFIG=build/raindrop-unofficial.mozconfig ./mach build
   for suite in AtURI DidResolver PdsClient HandleResolver RecordView CidVerifier ErrorView PrivacyFlags LexiconView; do
     ./mach gtest "$suite.*" || exit 1
   done
   ./mach xpcshell-test netwerk/protocol/atproto/tests/test_at_protocol.js
   ```

5. **Record and publish.** Update the version table and hash in
   `docs/BUILDING.md`, then push `upstream`, `main`, the new `firefox-$V` tag,
   and open a pull request for the rebased queue. A fork release is a separate,
   annotated `v<raindrop>-fx<firefox>` tag (for example `v0.1.0-fx161.0`) created
   only after the queue lands on `main`.

## When a rebase is expensive

ADR 0002 is to be reopened if a rebase repeatedly takes more than a working day
or conflicts concentrate in ADR 0001's core files. Record the time spent and the
conflicting files in the upgrade issue so that decision has evidence. ESR is the
documented fallback, not an ad hoc switch.
