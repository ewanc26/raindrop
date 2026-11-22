# ADR 0010: collection summary policy

- **Status:** Accepted
- **Date:** 2026-11-22
- **Issue:** [#97](https://github.com/ewanc26/raindrop/issues/97)
- **Related:** [ARCHITECTURE.md](../ARCHITECTURE.md),
  [ADR 0008](0008-at-document-customization.md),
  [ADR 0009](0009-richtext-facet-policy.md)

## Context

A collection listing fetches `com.atproto.repo.listRecords` and currently
renders only each record key and CID. The PDS response also carries a `value`
object, but `ParseCollectionResponse` discards it. A collection of posts is
therefore a list of opaque keys, forcing a user to open every record to learn
what it contains.

`value` is untrusted PDS data. It can contain arbitrary strings, control
characters, hostile markup and very large values. The generated document has an
opaque origin, no scripts and a strict CSP, so any summary must be local,
escaped and bounded.

## Decision

1. `ParseCollectionResponse` keeps three optional summary fields on each record:
   a validated NSID from `$type`, one short string field, and a bounded
   `createdAt` string.
2. The string field is chosen from the fixed key set `text`, `displayName`,
   `description`, `name` and `title`, in that order. The first non-empty field
   wins. Other fields are ignored.
3. Summary strings have control bytes replaced with spaces, are trimmed, and
   are capped at 160 bytes for the main text and 40 bytes for `createdAt`.
   Truncation stops at a UTF-8 character boundary and appends an ellipsis.
4. A `$type` is displayed only when it passes the existing `IsValidNsid` check.
   A missing, non-object or unhelpful `value` leaves the summary empty and does
   not fail the collection listing.
5. Rendering uses the existing escaped `hint` style. No field is interpolated
   as raw HTML, no raw JSON is embedded, and no image, blob or gateway request
   is added.
6. Record links continue to be built from the resolved DID, collection and
   record key. A `uri` returned by the PDS is validated but never used to build
   navigation.

## Consequences

- Collection pages become scannable without another network request.
- Hostile or oversized record values are bounded and escaped; malformed values
  degrade to no summary rather than a broken listing.
- The summary is deliberately small and not a full record view. Users still
  open a record for lexicon-specific rendering, provenance and raw JSON.
- The CSP, security headers, opaque origin, no-gateway policy and record link
  construction are unchanged.
