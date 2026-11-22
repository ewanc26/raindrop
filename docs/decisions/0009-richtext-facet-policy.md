# ADR 0009: richtext facet rendering policy

- **Status:** Accepted
- **Date:** 2026-11-22
- **Issue:** [#95](https://github.com/ewanc26/raindrop/issues/95)
- **Related:** [ARCHITECTURE.md](../ARCHITECTURE.md),
  [ADR 0007](0007-blob-and-authenticity-policy.md),
  [ADR 0008](0008-at-document-customization.md)

## Context

Posts in `app.bsky.feed.post` carry their visible text in a `text` string and
their links, mentions and hashtags in a parallel `facets` array. Each facet uses
UTF-8 byte offsets into the text and a `feature` object with a Lexicon `$type`.
Raindrop's bundled post view currently escapes and renders the text but discards
the facets, so users cannot follow links or mentions that the author marked up.

Facets are untrusted PDS data. A malformed record can claim unsafe URL schemes,
invalid DIDs, out-of-range offsets, overlapping ranges or feature types that
Raindrop does not understand. The generated document has an opaque origin, no
scripts and a strict CSP, so any new markup must preserve those properties.

## Decision

1. The post view renders facets locally into escaped HTML. It adds no script,
   remote stylesheet, background fetch or gateway.
2. `app.bsky.richtext.facet#link` becomes an anchor only when the target is a
   syntactically strict `http://` or `https://` URL with a non-empty authority.
   Userinfo, whitespace, control characters, quote characters, backslashes,
   malformed ports and URLs over 2048 bytes are rejected. Accepted anchors carry
   `rel="noopener noreferrer"`.
3. `app.bsky.richtext.facet#mention` becomes an `at://` anchor only when the DID
   passes the existing `IsValidDid` check and normalizes as an authority-only AT
   URI. Invalid DIDs remain escaped text.
4. `app.bsky.richtext.facet#tag` renders the original text slice inside a local
   `<span class="hint">`. Raindrop does not invent a search service URL. Tag
   values longer than 100 bytes or containing control characters are rejected.
5. Byte offsets are clamped to UTF-8 character boundaries. Ranges that are
   reversed, out of bounds or collapse after clamping are dropped. Valid ranges
   are sorted by start and end; later overlapping ranges are skipped so the
   result is deterministic and never duplicates text.
6. Every text slice, URL and DID is HTML-escaped. Unknown feature types and
   malformed facets are dropped, leaving the surrounding text visible and
   escaped.

## Consequences

- Post links and mentions become navigable without changing resolution
  authority. A link facet is an ordinary user-initiated external navigation; it
  is not fetched when the `at://` page is generated.
- A hostile PDS cannot inject markup, scripts, unsafe schemes or a fabricated
  search URL through facets. Invalid facets degrade to plain escaped text.
- UTF-8 text is not split into replacement characters by valid-looking but
  misaligned offsets.
- The generic field table, CSP, security headers, opaque origin and no-gateway
  policy are unchanged.
