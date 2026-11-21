# ADR 0007: blob and authenticity policy

- **Status:** Accepted
- **Date:** 2026-11-21
- **Issue:** [#89](https://github.com/ewanc26/raindrop/issues/89)
- **Related:** [ARCHITECTURE.md](../ARCHITECTURE.md), [ADR 0005](0005-identity-authority-and-freshness-signoff.md)

## Context

Record lexicons can reference blobs by CID. Raindrop has no content gateway, so
record fetches go to the DID's PDS. A blob reference has two reasonable
presentations: a CID plus a link to the resolved PDS's `getBlob` endpoint, or a
fetch that renders the blob inline.

The inline path emits an `<img>` pointing at `getBlob`. That request goes to the
same PDS, but the CID is only a label: the downloaded bytes are never hashed and
compared to it. Rendering the image without saying so can read as a verified
claim about its contents.

Record CID verification has a similar limit. It re-encodes the returned record
and compares the digest to the CID in the PDS response. A hostile PDS supplies
both, so a match proves integrity and self-consistency, not authorship or a
repository-commit signature.

## Decision

1. Inline image loading is off by default. `network.atprotocol.record.showImages`
   defaults to `false`, and the renderer structs default to not fetching images
   so callers must opt in explicitly.
2. Blob refs always show the CID and a `rel="noreferrer"` link to `getBlob` on
   the resolved PDS. No gateway or third-party CDN is used in either mode.
3. When images are enabled, they load lazily from that same PDS. The download is
   not checked against the CID, and the preference description says so.
4. The record view states that CID verification covers integrity and PDS
   self-consistency, not authorship or repository-commit authenticity. It also
   warns whenever no signed commit proof is checked, which is currently always.
5. Commit and signature verification are not implemented in this change.

## Consequences

- A clean profile makes no blob requests when a record page opens. Users who
  enable inline images accept direct requests to the record's PDS.
- Blob links, inline rendering and the integrity-not-authorship wording are
  covered by tests.
- A later change can add a verifying blob layer behind the same preference
  without changing the link format or the generated-document policy.
- Claims of authorship or repository authenticity remain unsupported.
