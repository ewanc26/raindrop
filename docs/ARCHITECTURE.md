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
4. **Record fetch.** Call `com.atproto.repo.getRecord` on the PDS for the
   collection and rkey. Blobs referenced by the record are fetched from the same
   PDS.
5. **Rendering.** Dispatch on the record's lexicon:
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
| `did-resolver` | DID method dispatch, caching, and revocation semantics. |
| `pds-client` | Service-endpoint discovery and repo record/blob fetches. |
| `record-view` | Lexicon-dispatched rendering, generic JSON view. |

## Trust and failure model

- **No gateway fallback.** If a DID or PDS is unreachable, raindrop says so. It
  never silently substitutes a third-party gateway, because that would defeat the
  point of the fork.
- **Honest provenance.** The view always shows which DID and PDS served the
  content and whether verification passed.
- **Local-only caching.** Resolution caches live on the user's machine, honour
  private browsing, and expire.

## Resolved design questions

- Minimal patch surface vs. deeper Gecko changes: [ADR 0001](decisions/0001-at-uri-scheme-registration.md).
- When handles resolve, caching and change detection:
  [ADR 0003](decisions/0003-handle-authority-semantics.md).
- `did:web` trust rules and onion PDS endpoints:
  [ADR 0004](decisions/0004-did-web-and-onion-endpoints.md).

## Open questions

- What minimum set of record lexicons gets a dedicated view in Phase 3?
