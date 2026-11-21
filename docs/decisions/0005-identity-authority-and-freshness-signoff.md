# ADR 0005: identity authority and handle freshness sign-off

- **Status:** Accepted
- **Date:** 2026-11-21
- **Issue:** [#6](https://github.com/ewanc26/raindrop/issues/6)
- **Related:** [ADR 0003](0003-handle-authority-semantics.md), [ARCHITECTURE.md](../ARCHITECTURE.md), [ROADMAP Phase 1](../ROADMAP.md)

## Context

Issue #6 asked for adversarial review of raindrop's trust and failure model.
The factual findings were resolved in code and docs through earlier PRs: the
"no gateway" claim was scoped to content, CID verification was relabelled as
integrity rather than authenticity, and the DNS leg's in-session caching was
documented precisely. Two decisions were deliberately left to a human because
they set project policy rather than follow from the spec. This ADR records the
sign-off on both.

## Decision

1. **`plc.directory` is an accepted trusted identity authority by design.**
   `BuildDidDocumentUrl` hardcodes it for every `did:plc`. The "no content
   gateway fallback" rule applies to content only; identity resolution for
   `did:plc` depends on this single third party. If it is unreachable, the load
   fails with `DidResolutionFailed` rather than falling back to a mirror. This
   is a deliberate dependency, not a tolerated gap.
2. **The shipped handle-freshness model is accepted.** HTTP identity fetches
   set `INHIBIT_PERSISTENT_CACHING | LOAD_BYPASS_CACHE`. The DNS TXT leg uses
   the platform resolver's in-memory, session-scoped cache. The reverse
   `alsoKnownAs` check fails closed. This combination catches handle
   reassignment without a second query on every navigation. No
   `RESOLVE_BYPASS_CACHE` change is made, and no application-level
   handle→DID cache is added.

## Consequences

- `did:plc` availability is coupled to `plc.directory`. An outage is an honest
  failure, not a silent substitution, and the UI already names the failure
  state.
- A handle change is normally seen at the next navigation, but may be delayed
  by the platform DNS cache's lifetime. The reverse binding check still
  prevents a stale or hostile TXT claim from serving another identity's
  repository, so the residual window is a freshness nuance rather than a
  trust hole.
- There is no cache to configure, expire, or clear for private browsing, so no
  new preference or privacy rule is introduced.
- ADR 0003's open cache question is closed in favour of the shipped behaviour;
  the ADR is marked accepted and no follow-up cache work is scheduled.

## Alternatives considered

- **Force `RESOLVE_BYPASS_CACHE` for the DNS leg.** Rejected: it would add a
  query on every navigation without improving the trust guarantee, because the
  reverse binding check already fails closed.
- **Add an application-level handle→DID cache with a TTL.** Rejected: it adds
  stale-content and private-browsing rules for no correctness benefit over
  re-resolution, and the reverse binding check already provides the needed
  safety property.
- **Route `did:plc` resolution through a fallback directory.** Rejected: it
  contradicts the honest-failure model and would make the identity dependency
  less visible.

## References

- [ADR 0003](0003-handle-authority-semantics.md) — handle-resolution and
  reverse-binding semantics
- [ARCHITECTURE.md](../ARCHITECTURE.md) — trust and failure model
