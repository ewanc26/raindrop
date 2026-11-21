# ADR 0006: strict Lexicon `at://` conformance

- **Status:** Accepted
- **Date:** 2026-11-21
- **Issue:** [#85](https://github.com/ewanc26/raindrop/issues/85)
- **Related:** [ADR 0001](0001-at-uri-scheme-registration.md), [ARCHITECTURE.md](../ARCHITECTURE.md)

## Context

The AT Protocol specification defines two `at://` grammars: a restricted
Lexicon `at-uri` form used in records, and a general form that permits query
and JSON Pointer fragments. Raindrop also has a private three-segment
`/cursor/<token>` navigation path used to paginate collection views.

A single permissive parser made these concerns indistinguishable. It accepted
an uppercase scheme, empty fragments and percent-encoded record keys, all of
which the canonical interop vectors reject. It also normalized record keys by
decoding unreserved percent escapes, even though record keys are case-sensitive
and must not be normalized.

## Decision

Raindrop has two parser entry points backed by one shared implementation:

1. `ParseAtURI` / `NormalizeAtURI` implement the restricted Lexicon grammar.
   They reject queries, fragments, uppercase schemes, duplicate slashes, dot
   segments, percent-encoded collection names and percent-encoded record keys.
   The authority retains DID-specific normalization and handle lowercasing.
2. `ParseNavigationAtURI` / `NormalizeNavigationAtURI` add only Raindrop's
   `/cursor/<token>` path segment. Browser navigation uses this mode; PDS
   response validation and lexicon record rendering use the strict mode.

The general AT URI grammar with query and fragment fields is not implemented in
this change. The private cursor path is not part of the AT URI grammar and is
never emitted by strict normalization.

DID validation rejects duplicate percent-encoding, and `did:web` authorities
are lowercased before hex-triplet normalization.

## Consequences

- The official CC0 conformance vectors are vendored into the gtest suite, with
  the one NSID vector that exceeds the specification's domain-authority limit
  explicitly skipped.
- Uppercase `AT://` input and fragment-bearing record references are no longer
  accepted. Gecko's scheme canonicalization means navigation still receives
  lowercase `at://`.
- Cursor links continue to work in the browser and are only accepted by the
  navigation parser.
- A future general-grammar parser can be added without weakening Lexicon
  record validation.
