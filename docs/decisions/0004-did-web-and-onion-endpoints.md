# ADR 0004: `did:web` and onion PDS endpoints

- **Status:** Proposed
- **Date:** 2026-11-20
- **Issue:** [#7](https://github.com/ewanc26/raindrop/issues/7)
- **Related:** [ARCHITECTURE.md](../ARCHITECTURE.md), [ADR 0003](0003-handle-authority-semantics.md), [ROADMAP Phase 2](../ROADMAP.md)

## Context

Two resolution paths need explicit treatment before Phase 2, as recorded in
issue #7:

- **`did:web`** resolves via `https://<domain>/.well-known/did.json`. Unlike
  `did:plc`, there is no independent directory: the domain in the DID is also
  the identity anchor, so whoever controls TLS for that domain controls the
  document. Redirects and URL-encoding ambiguities can silently move the
  lookup to another host or path.
- **Onion PDS endpoints.** Some PDSes are reachable only as `.onion` services.
  A browser with no Tor transport cannot reach them, and a "no gateway"
  fork must not quietly route them through a public clearinghouse.

The trust model in [ARCHITECTURE.md](../ARCHITECTURE.md) requires no gateway
fallback and honest provenance. Existing resolver code enforces `https` for
service endpoints and percent-decodes `did:web` segments.

## Decision

**Treat `did:web` as a TLS-anchored lookup with strict host/redirect rules, and
treat onion endpoints as an explicit, visible capability failure until native
Tor support exists.**

### `did:web`

1. **URL construction is canonical.** The method-specific identifier is
   percent-decoded one segment at a time. The first segment is the host, with
   an optional port encoded as `%3A`; later segments form the path. Traversal
   (`.`/`..`), userinfo, reserved characters after decoding, and ports outside
   1–65535 are rejected before any request is made.
2. **HTTPS only.** The document is fetched over `https://` with normal
   certificate verification. There is no plain-HTTP path and no certificate
   exception flow for `did:web`.
3. **Redirects may not change origin.** A redirect to a different host or port
   is a failed resolution. Same-origin redirects to a final path are allowed,
   but the fetched document's `id` must still match the requested DID — the
   check already performed by `ExtractPdsEndpoint`. This keeps the domain in
   the DID as the identity anchor.
4. **Provenance.** The provenance bar shows `did:web:<domain>` and the exact
   document URL, so the user can see which domain anchored the identity.

### Onion endpoints

5. **No native Tor transport in Phase 1/2.** raindrop does not bundle or launch
   Tor, so `.onion` service endpoints cannot be fetched. This is a conscious
   coverage decision, not an oversight.
6. **No gateway fallback.** An onion endpoint never gets proxied through a
   public gateway or a third-party Tor service. Doing so would both defeat the
   no-gateway guarantee and leak the requested identity to that service.
7. **Honest failure.** When a DID document names an `.onion` PDS, the load
   fails with a specific state: "PDS is onion-only; native Tor support is not
   available". The provenance bar still displays the DID and the onion
   endpoint that was requested.
8. **Future support.** Native onion support, if added, must keep the fetch
   inside raindrop's own network stack and preserve the same provenance
   display. It reopens this ADR rather than being assumed.

## Consequences

- `did:web` identities whose documents redirect cross-origin become unusable
  in raindrop. That is the intended trade: the domain in the DID is the trust
  root, so a cross-origin redirect breaks the identity claim.
- Onion-only repositories are not viewable in Phase 1/2. Users who need them
  must use a separate Tor-enabled browser; raindrop will not pretend otherwise.
- The `https` and no-redirect rules are enforced in the resolver and channel
  code, not left to the user, so there is no unsafe opt-out.

## Alternatives considered

- **Follow cross-origin `did:web` redirects.** Rejected: it lets a compromised
  or careless domain move the identity document off the domain the DID names.
- **Proxy onion endpoints through a gateway.** Rejected: violates the
  no-gateway guarantee and leaks browsing information to the proxy.
- **Bundle Tor now.** Deferred: materially larger scope and a separate
  security/UX review; it belongs in its own decision.
- **Reject `did:web` entirely.** Rejected: it is a common DID method and the
  strict rules above preserve its usefulness without weakening trust.

## References

- [ARCHITECTURE.md](../ARCHITECTURE.md) — trust and failure model, no gateway fallback
- [ADR 0003](0003-handle-authority-semantics.md) — reverse binding for handle authorities
- [did:web specification](https://w3c-ccg.github.io/did-method-web/) — well-known DID document resolution
- [AT Protocol DID](https://atproto.com/specs/did) — `did:plc` and `did:web` usage
