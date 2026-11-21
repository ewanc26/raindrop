# Architecture

This document describes how raindrop handles `at://`. Design decisions are
recorded in the linked ADRs.

## The life of an `at://` request

```
at://did:plc:abc.../app.bsky.feed.post/3lz...
        |            |                    |
     authority    collection             rkey
   (DID or handle)
```

1. **Scheme dispatch.** The address bar hands `at://` URLs to the `atproto/`
   module instead of the HTTP stack. The authority is kept opaque during
   parsing; it may be a DID or a handle. Handle authorities are resolved to a
   DID before the repository fetch, but the URI itself is never rewritten.
2. **DID resolution.** Method dispatch on the authority:
   - `did:plc` - look up the DID document at the PLC directory.
   - `did:web` - fetch `https://<domain>/.well-known/did.json`.
3. **PDS discovery.** Read the `#atproto_pds` service endpoint from the DID
   document. This is where the record lives.
4. **Fetch.** A full record URI calls `com.atproto.repo.getRecord` on the PDS
   for the collection and rkey. An authority-only URI calls
   `com.atproto.repo.describeRepo` to describe the repository, and a
   collection URI calls `com.atproto.repo.listRecords`. A page token from a
   PDS cursor is carried as an explicit `at://did/collection/cursor/<token>`
   segment; query parameters are not part of the AT-URI grammar, and a
   fragment-only change would be treated as a same-document navigation instead
   of a new fetch.
5. **Rendering.** A repository description renders a summary whose advertised
   collections link to collection listings. A collection listing renders
   escaped record links built from the resolved DID, never from the URI the
   PDS returned. Record responses dispatch on lexicon:
   - Known lexicons (Phase 3) get a bundled native view. Reply parents, roots
     and quoted records are validated with `ParseAtURI` and rendered as
     normalized `at://` links. Like and repost subjects get the same treatment;
     follow subjects are validated with `IsValidDid` and rendered as
     authority-only `at://` links.
   - Everything else gets the generic record view (Phase 1) with the option to
   inspect raw JSON.
6. **Verification (Phase 2).** The fetched bytes can be checked against the
   record's CID so the viewer sees the content at the exact data version.

## Why in the browser, not an extension

Existing `at://` extensions capture the URI outside the address bar (clipboard,
shortcuts or redirects) and open a web viewer. That still routes content through
a gateway. The limits are structural:

1. **No custom scheme registration.** WebExtensions cannot make a new URI
   scheme navigable. An `at://` string typed in the address bar is resolved by
   the browser before extension code runs. `navigator.registerProtocolHandler`
   covers a fixed allowlist of schemes, is site-initiated, and still lands the
   user on an HTTP page.
2. **The URL parser cannot be changed by an extension.** Because DID
   authorities embed un-encoded colons (`at://did:plc:...`), the URI does not
   conform to the WHATWG URL standard (noted in the IANA registration and the
   IETF drafts — see TRADEMARKS.md's footnote). The parser that has to cope with
   that is the engine's URI machinery (`nsIURI` and friends), which extensions
   cannot replace.
3. **CORS and network-stack fetches.** A fetch from extension or page context
   against an arbitrary PDS only works if that PDS opts in via CORS. The
   protocol should not require every PDS operator to send
   `Access-Control-Allow-Origin` to every viewer. The browser's fetcher brings
   its own cache, connection pooling and private-browsing semantics.
4. **Provenance must live in trusted UI.** Provenance (which DID, which PDS,
   whether the CID verified, which resolution step failed) is part of the
   browser's security UI. Only browser chrome can render it in a way the
   content being viewed cannot forge.
5. **Browser-level integration.** `at://` in bookmarks, history, link
   handling across tabs, and error pages all expect the same behaviour as other
   navigable schemes. Implementing it as a convention on top of HTTP pages
   would leave those seams visible.

For these reasons, the scheme, parser and fetch path live in the browser.

## Modules

| Module | Responsibility |
| ------ | -------------- |
| `at-uri` | Parse and normalise strict Lexicon `at://` URIs (authority, collection, rkey) and Raindrop's navigation-only cursor extension. |
| `did-resolver` | DID method dispatch and DID-document validation. |
| `pds-client` | Service-endpoint discovery and repo record fetches. |
| `record-view` | Lexicon-dispatched rendering, generic JSON view. |

## Trust and failure model

- **No content gateway fallback.** If a DID or PDS is unreachable, raindrop says
  so. It never silently substitutes a third-party gateway, because that would
  defeat the point of the fork. This is a claim about *content* only: resolving
  a `did:plc` identity requires `plc.directory`, which is hardcoded in
  `BuildDidDocumentUrl` and accepted as a trusted identity authority by design.
  If it is unreachable, `did:plc` loads fail with `DidResolutionFailed` rather
  than falling back to a mirror.
- **Honest provenance.** The view shows which DID and PDS served the content and
  the outcome of CID verification. That check re-encodes the returned record as
  canonical DAG-CBOR and compares its SHA-256 with the CID returned alongside
  it; it proves *integrity* (the record matches its CID) and consistency with
  the PDS response, not *authenticity* (a repository signature or proof of
  authorship). Raindrop does not check commit signatures or signed commit
  proofs. A hostile PDS controls both the record and its CID, so the view
  labels this limit explicitly.
- **Local-only caching.** HTTP identity fetches set
  `INHIBIT_PERSISTENT_CACHING | LOAD_BYPASS_CACHE`, so they are never written to
  disk and are re-fetched on each navigation. The DNS TXT leg of handle
  resolution uses the platform resolver, whose cache is in-memory for the
  session only; it is likewise never persisted. Handle reassignment is caught by
  the reverse `alsoKnownAs` check, which fails closed.

## Resolved design questions

- Minimal patch surface vs. deeper Gecko changes: [ADR 0001](decisions/0001-at-uri-scheme-registration.md).
- Strict Lexicon conformance vs. general AT URI syntax:
  [ADR 0006](decisions/0006-strict-lexicon-at-uri-conformance.md).
- Blob rendering and the limits of CID verification:
  [ADR 0007](decisions/0007-blob-and-authenticity-policy.md).
- When handles resolve, caching and change detection:
  [ADR 0003](decisions/0003-handle-authority-semantics.md).
- Design sign-off on `plc.directory` as identity authority and the shipped
  handle-freshness model:
  [ADR 0005](decisions/0005-identity-authority-and-freshness-signoff.md).
- `did:web` trust rules and onion PDS endpoints:
  [ADR 0004](decisions/0004-did-web-and-onion-endpoints.md).
- Minimum set of record lexicons with a dedicated view: `app.bsky.feed.post`,
  `app.bsky.actor.profile`, `app.bsky.feed.like`, `app.bsky.feed.repost` and
  `app.bsky.graph.follow`, with the generic field table as fallback.
  Blob references show their CID and link to the resolved PDS's `getBlob`
  endpoint. Inline image loading is off by default; when enabled with
  `network.atprotocol.record.showImages`, images are fetched lazily from that
  same endpoint and the CID is not checked against the downloaded bytes. The
  CID and link are shown in either mode. See
  [ADR 0007](decisions/0007-blob-and-authenticity-policy.md).
