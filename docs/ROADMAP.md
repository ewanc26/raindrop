# Roadmap

raindrop is developed in phases. Each phase ends with something that can actually be
used, even if it is small.

## Phase 0 - planning (current)

- [ ] Feasibility & decisions record: which Firefox source to track
      (`mozilla-central` vs release channel), branch/tag strategy, release cadence.
      Proposed in [ADR 0002](decisions/0002-upstream-tracking.md).
- [ ] Build bootstrap: reproduce a stock Firefox build before any patches, and a CI
      job that builds it on every change to the fork layer.
- [ ] Design review of [ARCHITECTURE.md](ARCHITECTURE.md), especially the trust and
      failure models.
- [ ] Decide the minimal patch surface: keep changes in an isolated `atproto/`
      module so rebases onto upstream are as small as possible.

## Phase 1 - first rain (M1)

- [ ] `at://` is a registered scheme: typed into the address bar it reaches
      raindrop's resolver instead of a search query.
- [ ] DID resolution: `did:plc` via plc.directory, `did:web` via the
      `/.well-known/did.json` convention, with method dispatch for future methods.
- [ ] PDS discovery from the DID document's service endpoint, and
      `com.atproto.repo.getRecord` fetches.
- [ ] A generic, unstyled record view (lexicon, fields, CID, raw JSON) so *every*
      `at://` URI is viewable from day one.

## Phase 2 - trust (M2)

- [ ] Data integrity: content fetched from PDSes can be verified against its CID.
- [ ] Clear provenance UI: which DID, which PDS, and whether resolution succeeded
      end to end; honest failure states (unknown DID, PDS down, record deleted).
- [ ] Privacy defaults: no fallback gateway, no third-party fetch to "fix" a lookup;
      caching stays local and respects private-browsing mode.

## Phase 3 - grown (M3)

- [ ] Bundled views for common lexicons (posts, profiles, blobs) rendered natively.
- [ ] Browser integration polish: `at://` in bookmarks, history, and link handling.
- [ ] Fork maintenance automation: scripted rebases onto upstream Firefox releases
      and an upgrade CI job, so the fork never rots.
- [ ] First tagged release.
