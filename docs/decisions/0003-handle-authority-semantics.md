# ADR 0003: handle-authority semantics

- **Status:** Accepted
- **Date:** 2026-11-20
- **Issue:** [#5](https://github.com/ewanc26/raindrop/issues/5)
- **Related:** [ARCHITECTURE.md](../ARCHITECTURE.md), [ADR 0001](0001-at-uri-scheme-registration.md), [ADR 0005](0005-identity-authority-and-freshness-signoff.md), [ROADMAP Phase 1](../ROADMAP.md)

## Context

An AT-URI authority may be a DID or a handle. Handles are human-readable but
not durable: the owner of `alice.example` can change, and a reused handle can
point at a different repository. The atproto specification warns about exactly
this, and issue #5 asks when raindrop resolves a handle, whether it caches the
mapping, how it detects a change, and what the user sees.

The two competing goals are:

1. **Correctness.** The record must come from the repository the handle
   denoted *now*, not a repository an old cached mapping happened to point at.
2. **Honesty.** The user must be able to tell that they followed a handle
   rather than a DID, and which DID it resolved to.

ADR 0001 already fixes the parsed authority as an opaque string, so resolution
can happen later without rewriting the URI.

## Decision

**Resolve handle authorities on every navigation, verify the binding in both
directions, cache only in memory for the lifetime of the session, and show a
durability warning. Never persist the mapping.**

1. **When.** A handle authority is resolved during the load that needs it,
   before the repository fetch. The original `at://` URI is never rewritten to
   the DID form; the address bar and history keep what the user typed.
2. **How.** Resolve the handle to a DID using the atproto handle-resolution
   conventions: a DNS TXT record at `_atproto.<handle>` of the form
   `did=<did>`, or an HTTPS GET to `https://<handle>/.well-known/atproto-did`
   returning the DID as plain text. Then **verify the reverse binding**: fetch
   the resolved DID document and require its first syntactically valid
   `at://` entry in `alsoKnownAs` to be `at://<handle>`. Per the atproto DID
   spec, later entries are not handle claims and are ignored. A handle that
   resolves to a DID whose document does not claim it is a failed resolution,
   not a warning. This blocks a stale or hostile handle record from redirecting
   the viewer to an unrelated repo.
3. **Caching.** No application-level handle→DID cache is implemented. Each
   navigation re-resolves the handle, and HTTP identity fetches set
   `INHIBIT_PERSISTENT_CACHING | LOAD_BYPASS_CACHE` so nothing is written to
   disk. The only in-memory caching is the platform DNS resolver's, which is
   session-scoped and never persisted. This shipped model was accepted by
   sign-off in [ADR 0005](0005-identity-authority-and-freshness-signoff.md);
   no application-level cache is planned.
4. **Change detection.** Because resolution happens on navigation, a changed
   handle is seen at the next load. If the reverse check fails, the load fails
   with an explicit "handle no longer resolves to this identity" state; the
   user is never silently shown content from the new owner.
5. **UI.** A handle-authority URI renders a small, non-alarming durability hint
   next to the provenance bar: "handle — may change; resolved to <did>". The
   DID is always displayed so the user can pin the durable form if they want.
   This mirrors the atproto caveat without blocking navigation.
6. **Failure model.** If handle resolution or reverse verification fails, the
   load fails honestly. Per the trust model there is no gateway fallback and
   no attempt to guess a DID from the handle. An ambiguous DNS TXT set — more
   than one `did=` claim, even if some are malformed — is rejected outright
   rather than selected from or allowed to fall through to the HTTPS path.

## Consequences

- Every handle navigation costs a DNS or HTTPS lookup plus a DID-document
   fetch. That is acceptable: handles are the convenient, non-durable path,
   and DIDs remain the fast path.
- Reverse verification means a misconfigured DID document makes a handle
   unusable in raindrop even if another viewer would accept it. This is
   intentional; the cost of a false positive is showing the wrong person's
   data.
- No application-level cache exists, so there is no TTL or private-browsing
   rule to maintain. [ADR 0005](0005-identity-authority-and-freshness-signoff.md)
   records the accepted freshness model.

## Alternatives considered

- **Persist a handle→DID map.** Rejected: a stale entry is indistinguishable
  from a current one without re-resolution, and a reused handle could then
  serve the old owner's data indefinitely.
- **Resolve once at startup.** Rejected: misses changes during a long session
  and adds startup work for handles the user never visits.
- **Skip reverse verification.** Rejected: it is the check that binds the
  human-readable name to the DID document, and omitting it defeats the
  durability warning's purpose.
- **Rewrite the URI to the DID form.** Rejected: loses what the user typed and
  contradicts ADR 0001's opaque-authority design.

## References

- [ARCHITECTURE.md](../ARCHITECTURE.md) — life of an `at://` request, trust and failure model
- [ADR 0001](0001-at-uri-scheme-registration.md) — opaque authority, minimal patch surface
- [atproto handle resolution](https://atproto.com/specs/handle) — DNS TXT and `/.well-known/atproto-did` conventions
- [atproto DID](https://atproto.com/specs/did) — `alsoKnownAs` and identity binding
