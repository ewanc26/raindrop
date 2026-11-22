# Verifying AT Protocol in a local build

This is the AT Protocol companion to [BUILDING.md](BUILDING.md). It covers local
verification of `at://` resolution without a content gateway.

## 1. Build

Follow [BUILDING.md](BUILDING.md#bootstrap-and-build), then choose the branding
you want to test:

```sh
export MOZCONFIG="$PWD/build/raindrop-unofficial.mozconfig"
./mach build
./mach run
```

Use `build/raindrop.mozconfig` instead when checking the shipped branding.

## 2. Verify the wiring offline

The verifier checks the scheme registration, opaque-authority pref, AT prefs,
settings pane, build wiring, test entry points and CI commands without starting
the browser:

```sh
python3.12 tools/raindrop/verify_atproto_setup.py
```

It exits non-zero with a list of missing or forbidden settings. The same check
runs in the hosted offline suite.

## 3. Run the AT test suites

Use one colon-separated GTest argument; separate invocations can hide failures:

```sh
export MOZCONFIG="$PWD/build/raindrop.mozconfig"
python3.12 ./mach gtest 'AtDocumentPolicy.*:AtURI.*:DidResolver.*:PdsClient.*:HandleResolver.*:RecordView.*:CidVerifier.*:ErrorView.*:PrivacyFlags.*:LexiconView.*:AtRecordResolver.*:RepoView.*:CollectionView.*'
python3.12 ./mach xpcshell-test netwerk/protocol/atproto/tests/test_at_protocol.js
python3.12 ./mach xpcshell-test browser/components/places/tests/unit/test_browserGlue_prefs.js
python3.12 ./mach mochitest browser/components/places/tests/browser/browser_bookmark_current_tabs.js --headless
python3.12 ./mach mochitest browser/components/places/tests/browser/browser_bookmarks_checkDefaultBookmarks.js --headless
python3.12 ./mach mochitest browser/components/preferences/tests/browser_at_protocol_pane.js --headless
python3.12 ./mach mochitest netwerk/protocol/atproto/tests/browser/browser_at_protocol_navigation.js --headless
python3.12 -m unittest discover -s tools/raindrop/tests
```

The expected baseline is 136 GTests, 56 atproto xpcshell subtests, 21
browser-pref xpcshell subtests and four mochitests. Exact counts are recorded in
[RELEASING.md](RELEASING.md).

## 4. Smoke test in the browser

1. Start `./mach run`.
2. Type `at://ewancroft.uk` in the address bar. It must be classified as a URL,
   not a search. A successful load renders a repository summary with the
   resolved handle, DID, PDS and advertised collections.
3. Open a collection from that summary, then a record. Provenance must show the
   DID, PDS and CID result. Failures render a step-specific HTML page rather
   than a blank tab.
4. Follow a cursor link from a collection listing and confirm the next page
   loads. The cursor path is Raindrop's navigation extension, not a canonical
   AT URI.
5. Open `about:preferences#atProtocol`, or use the **AT Protocol** default
   bookmark. Toggle `network.atprotocol.enabled` off and repeat the navigation;
   it must fail cleanly. Toggle it back on.
6. Right-click an `at://` link and confirm Copy Link Address and Save Link As
   use the raw `at://` spec. Reader mode is not offered; Raindrop renders its
   own views.
7. Open a post with an image. With `network.atprotocol.record.showImages` off,
   no image request is made; the CID and a PDS `getBlob` link are shown. Turn
   the pref on and reload to see the image load inline.
 8. In `about:preferences#atProtocol`, change **Page colour theme** to `Dark` or
    `High contrast` and reload an `at://` page. Text size, page width, link
    colour and spacing each apply to record, repository, collection and error
    pages. These five settings are local and make no network request. **Records
    per collection page** changes the `limit` sent to the PDS for collection
    listings.

No load should contact an HTTP gateway for content. `plc.directory` and DNS are
identity-resolution authorities, not content gateways.

## 5. Security behaviour

- `at://` has an opaque origin (`window.origin === "null"`) and is not a secure
  context. Identity UI must not claim a verified web domain.
- Web pages can follow `at://` links, but scripted `fetch("at://...")` access is
  not part of the navigation contract.
- Record CID verification covers integrity and consistency with the PDS
  response, not authorship or repository-commit authenticity. The view states
  that no signed commit proof is checked.
- Inline image loading is off by default. Blob refs show a CID and a link to
  the record's PDS. Enabling `network.atprotocol.record.showImages` fetches
  images directly from that PDS without checking the bytes against the CID.
- Generated documents carry hardening headers and matching meta tags, and
  internal DID/PDS fetches omit cookies and referrers.
- Appearance settings choose from fixed values; unrecognized values fall back to
  the shipped default instead of entering the generated CSS. Style blocks contain
  no scripts, remote URLs or `url()` references.

## Troubleshooting

| Symptom | Check |
| --- | --- |
| `at://...` becomes a search | `python3.12 tools/raindrop/verify_atproto_setup.py`; the `at` handler must be registered |
| Load fails with unavailable | `network.atprotocol.enabled` must be true |
| `did:plc` load fails | Network access to `plc.directory`; Raindrop intentionally has no fallback gateway |
| Handle load fails | DNS TXT `_atproto.<handle>` or `https://<handle>/.well-known/atproto-did`, plus reverse `alsoKnownAs` confirmation |
| Very long URI navigates but is not stored | Places history has a 2000-byte URL limit; the AT parser allows up to 8192 bytes |
