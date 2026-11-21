# Roadmap

raindrop is developed in phases. Each phase ends with something that can actually be
used, even if it is small.

## Phase 0 - planning (current)

- [x] Feasibility & decisions record: which Firefox source to track
      (`mozilla-central` vs release channel), branch/tag strategy, release cadence
      ([ADR 0002](decisions/0002-upstream-tracking.md)).
- [ ] Build bootstrap: reproduce a stock Firefox build before any patches, and a CI
      job that builds it on every change to the fork layer. The stock build is
      reproduced locally with the tracked `build/raindrop-unofficial.mozconfig`
      and its observed cost is recorded in [BUILDING.md](BUILDING.md). The CI job
      exists in `.github/workflows/build-bootstrap.yml` but is gated on a
      self-hosted runner (`RAINDROP_BUILD_RUNNER`); it cannot be proven on a
      hosted runner with 14 GB of disk, so this stays open until one is
      registered.
- [x] Design review of [ARCHITECTURE.md](ARCHITECTURE.md), especially the trust and
      failure models. Handle authorities ([ADR 0003](decisions/0003-handle-authority-semantics.md)),
      `did:web`/onion endpoints ([ADR 0004](decisions/0004-did-web-and-onion-endpoints.md))
      and the signed-off identity/freshness model
      ([ADR 0005](decisions/0005-identity-authority-and-freshness-signoff.md))
      are recorded; no design questions remain open.
- [x] Decide the minimal patch surface: keep changes in an isolated `atproto/`
      module so rebases onto upstream are as small as possible
      ([ADR 0001](decisions/0001-at-uri-scheme-registration.md)).

## Phase 1 - first rain (M1)

- [x] `at://` is a registered scheme: typed into the address bar it reaches
      raindrop's resolver instead of a search query. The scheme is registered from
      the isolated `atproto/` module with an opaque authority ([ADR 0001](decisions/0001-at-uri-scheme-registration.md)).
- [x] DID resolution: `did:plc` via plc.directory, `did:web` via the
      `/.well-known/did.json` convention, with method dispatch for future methods.
      Parsing, URL construction and DID-document validation are implemented with
      GTest coverage, and the network fetch is wired into the channel.
- [x] PDS discovery from the DID document's service endpoint, and
      `com.atproto.repo.getRecord` fetches. HTTPS-only endpoints, no gateway
      fallback, cross-origin redirects rejected ([ADR 0004](decisions/0004-did-web-and-onion-endpoints.md)).
- [x] Handle authorities resolve via DNS TXT at `_atproto.<handle>` with an
      HTTPS `/.well-known/atproto-did` fallback, reverse `alsoKnownAs`
      verification and no persistent cache ([ADR 0003](decisions/0003-handle-authority-semantics.md)).
      The record view shows a durability hint for handle-based authorities.
- [x] A generic, unstyled record view (lexicon, fields, CID, raw JSON) so *every*
      `at://` URI is viewable from day one. The resolver renders an escaped HTML
      view of any record, with provenance and raw JSON, with no lexicon-specific
      viewer required.

## Phase 2 - trust (M2)

- [x] Data integrity: content fetched from PDSes can be verified against its CID.
      The record value is re-encoded as canonical DAG-CBOR and its SHA-256 digest
      compared with the CID; the result is shown as provenance metadata.
- [x] Clear provenance UI: which DID, which PDS, and whether resolution succeeded
      end to end; honest failure states (unknown DID, PDS down, record deleted).
      The success view shows DID/PDS/CID and verification outcome; failures render
      a step-specific HTML page distinguishing handle, DID, PDS and record errors.
- [x] Privacy defaults: no fallback gateway, no third-party fetch to "fix" a lookup;
      caching stays local and respects private-browsing mode. Resolution fetches
      set INHIBIT_PERSISTENT_CACHING | LOAD_BYPASS_CACHE, so identity lookups are
      never written to disk and are re-resolved on every navigation; private
      browsing is inherited from the loadInfo's origin attributes.

## Phase 3 - grown (M3)

- [x] Bundled views for common lexicons (posts, profiles) rendered natively, with
      escaped, friendly summaries layered above the generic view; unknown lexicons
      fall back to the generic field table. Blob rendering remains a follow-up.
- [x] Browser integration polish: `at://` in bookmarks, history, and link handling.
      The scheme is `URI_LOADABLE_BY_ANYONE` (persistable, not dangerous to load),
      so web content can follow `at://` links and the URI is eligible for
      bookmarks/history; the address bar classifies it as a URL (ADR 0001).
- [x] Fork maintenance automation: scripted rebases onto upstream Firefox releases
      and an upgrade CI job, so the fork never rots. `tools/raindrop/upstream.py`
      detects a due upgrade against Mozilla's release channel (44 offline tests),
      `.github/workflows/upgrade-check.yml` opens/refreshes a triage issue daily,
      and [docs/UPGRADING.md](UPGRADING.md) records the signed import + rebase
      procedure per ADR 0002. The signed 160.0.1 release was imported and tagged.
- [ ] First tagged release.
