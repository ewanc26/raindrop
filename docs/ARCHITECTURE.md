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
   module instead of the HTTP stack. Handles (`at://ewancroft.uk`) are resolved
   to DIDs first, since the authority in an `at://` URI must be a DID.
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

## Open questions

- How much of this can live behind a small patch surface vs. needing deeper
  gecko changes to `nsIURI` / scheme registration?
- Should handle resolution go through DNS/handle conventions at startup, and how
  is a handle change detected?
- What is the right story for `did:web` and Tor/ onion PDS endpoints?
