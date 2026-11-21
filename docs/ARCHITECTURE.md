# Architecture

This document is the working design for how raindrop handles `at://`. It is a
review draft; challenge it in issues.

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
   collection URI calls `com.atproto.repo.listRecords`.
5. **Rendering.** A repository description renders a summary whose advertised
   collections link to collection listings. A collection listing renders
   escaped record links built from the resolved DID, never from the URI the
   PDS returned. Record responses dispatch on lexicon:
   - Known lexicons (Phase 3) get a bundled native view.
   - Everything else gets the generic record view (Phase 1) with the option to
   inspect raw JSON.
6. **Verification (Phase 2).** The fetched bytes can be checked against the
   record's CID so the viewer sees the content at the exact data version.

## Why in the browser, not an extension

An extension is the natural first idea, and the ecosystem has tried it: existing
`at://` extensions work by capturing the URI somewhere other than the address
bar (clipboard, shortcuts, redirects) and opening a compatible web viewer. That
is a gateway with extra steps, and the rest is a list of hard WebExtension
limits:

1. **No custom scheme registration.** WebExtensions cannot make a new URI
   scheme navigable. An `at://` string typed in the address bar is resolved by
   the browser itself (search, or an OS handler) before any extension code runs,
   so the extension model can't even enter the room. `navigator.registerProtocolHandler`
   exists, but it covers a fixed allowlist of schemes, is site-initiated, and
   still lands the user on an HTTP page — a gateway.
2. **The URL parser is not extensible.** Because DID authorities embed
   un-encoded colons (`at://did:plc:...`), the URI does not conform to the
   WHATWG URL standard (noted in the IANA registration and the IETF drafts —
   see TRADEMARKS.md's footnote). The parser that has to cope with that is the
   engine's URI machinery (`nsIURI` and friends), which extensions cannot
   touch. raindrop needs to *fix the parsing*, not work around it.
3. **CORS and network-stack fetches.** A fetch from extension/page context
   against an arbitrary PDS only works if that PDS opts in via CORS. The
   protocol should not require every PDS operator to send
   `Access-Control-Allow-Origin` to every viewer origin. The browser's own
   fetcher has no such constraint and brings the HTTP cache, connection
   pooling, and private-browsing semantics along.
4. **Provenance must live in trusted UI.** Honest provenance (which DID, which
   PDS, whether the CID verified, which resolution step failed) is core
   security UX, not decoration. Only browser chrome can render it in a way the
   content being viewed cannot forge.
5. **Browser-level integration.** `at://` in bookmarks, history, link
   handling across tabs, and error pages all expect first-class URIs; being a
   convention bolted onto top-level HTTP pages would leak those seams
   everywhere.

So: **an extension could deliver `at://`-as-gateway; only the browser can
deliver `at://`-as-infrastructure.** raindrop wants the second one.

## Modules

| Module | Responsibility |
| ------ | -------------- |
| `at-uri` | Parse and normalise `at://` URIs (authority, collection, rkey, fragment). |
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
  it; it proves *integrity* (the record matches its CID), not *authenticity* (a
  repository signature or proof of authorship). A hostile PDS controls both, so
  the view labels this limit explicitly.
- **Local-only caching.** HTTP identity fetches set
  `INHIBIT_PERSISTENT_CACHING | LOAD_BYPASS_CACHE`, so they are never written to
  disk and are re-fetched on each navigation. The DNS TXT leg of handle
  resolution uses the platform resolver, whose cache is in-memory for the
  session only; it is likewise never persisted. Handle reassignment is caught by
  the reverse `alsoKnownAs` check, which fails closed.

## Resolved design questions

- Minimal patch surface vs. deeper Gecko changes: [ADR 0001](decisions/0001-at-uri-scheme-registration.md).
- When handles resolve, caching and change detection:
  [ADR 0003](decisions/0003-handle-authority-semantics.md).
- Design sign-off on `plc.directory` as identity authority and the shipped
  handle-freshness model:
  [ADR 0005](decisions/0005-identity-authority-and-freshness-signoff.md).
- `did:web` trust rules and onion PDS endpoints:
  [ADR 0004](decisions/0004-did-web-and-onion-endpoints.md).
- Minimum set of record lexicons with a dedicated view: `app.bsky.feed.post`
  and `app.bsky.actor.profile`, with the generic field table as fallback;
  blob rendering is deferred.
