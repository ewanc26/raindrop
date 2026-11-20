# ADR 0002: Upstream Firefox tracking and branch strategy

- **Status:** Proposed
- **Date:** 2026-11-20
- **Issue:** [#8](https://github.com/ewanc26/raindrop/issues/8)
- **Related:** [ROADMAP Phase 0](../ROADMAP.md), [#10](https://github.com/ewanc26/raindrop/issues/10), [#12](https://github.com/ewanc26/raindrop/issues/12), [TRADEMARKS.md](../TRADEMARKS.md)

## Context

raindrop is a Firefox fork maintained by one person. The cost of a fork is not
the first patch; it is every rebase after it. The upstream source, the cadence
at which it is imported, and the shape of the patch queue therefore have to be
decided before any code lands.

Two facts have changed since this project was scoped:

1. **Firefox now ships every two weeks.** Mozilla moved Nightly, Beta and
   Release from a four-week to a two-week cycle beginning with Firefox 155 on
   1 September 2026
   ([Mozilla Support blog](https://blog.mozilla.org/sumo/2026/08/19/firefox-new-release-cadence-and-what-to-expect),
   [release process](https://wiki.mozilla.org/Release_Management/Release_Process)).
   As of 20 November 2026 the release channel is Firefox 160, Beta is 161
   (shipping 24 November), Nightly is 163, and the current ESR is 153
   ([whattrainisitnow](https://whattrainisitnow.com/)). The release channel
   supports only the current version; 160 gives way to 161 on 24 November.
2. **Git is the source of truth.** Firefox's canonical repository moved to
   `mozilla-firefox/firefox` on GitHub in April 2025, with `main`, `beta`,
   `release` and `esr153` branches
   ([repository](https://github.com/mozilla-firefox/firefox)). Signed release
   source tarballs are still published under
   `archive.mozilla.org/pub/firefox/releases/<version>/source/`; both
   `firefox-160.0.source.tar.xz` and its detached `.asc` signature are live
   ([releases](https://archive.mozilla.org/pub/firefox/releases/)). The Git
   repository is convenient for history and diffs, but the signed release
   artifact is the thing a released build is actually made from.

ESR is the third option. An ESR line is supported for more than a year and
receives security-only point releases on the regular release days, with at
least a twelve-week overlap before the previous line reaches end of life
([ESR release cycle](https://support.mozilla.org/en-US/kb/firefox-esr-release-cycle)).
That is a materially different maintenance bargain from the release channel.

The other constraints are fixed: one maintainer, a patch surface that should
stay as small as possible (ADR 0001), MPL-2.0 code with no Mozilla trademark
rights ([TRADEMARKS.md](../TRADEMARKS.md)), and a build that must be
reproducible before features are attempted (#10).

## Options

| Option | What it provides | What it costs |
| --- | --- | --- |
| `main` (Nightly) | Newest engine and APIs; easiest access to unreleased platform changes | A moving target every day; no supported-release guarantee; constant rebases; not what users run |
| `release` tags / source tarballs | Stable, signed, security-patched releases on a predictable two-week train | A rebase and preferably a release every two weeks; only the current version is supported |
| `esr153` | Security-only updates for more than a year; fewer rebases | An engine roughly sixteen weeks behind release today; a large annual rebase cliff when the ESR line changes |

## Decision

**Track the release channel, pinned to signed release source, and rebase on
every Firefox release (currently every two weeks).**

1. **Source.** Import the signed `firefox-<version>.source.tar.xz` release
   tarball for each Firefox release. Verify the detached signature against
   Mozilla's release key and record the SHA-256. The primary key is
   `14F2 6682 D091 6CDD 81E3 7B6D 61B7 B526 D98F 0353` (Mozilla Software
   Releases); at the time of writing the signing subkey is
   `827E 6586 0867 9618 CD34 9F93 678E 455D 7676 7AA3`. Fetch the current key
   from Mozilla's own `https://packages.mozilla.org/rpm/firefox/signing-key.gpg`
   and check fingerprints before trusting it. Do not follow `main`, `beta`,
   or the head of `release`; a mid-cycle commit is not a released artifact and
   carries no support promise. The Git repository may be used to inspect
   history and prepare patches, but the tarball wins if the two disagree.
2. **Cadence.** Rebase the patch queue after every Firefox release, within five
   working days. For a dot release, import it only when it contains a security
   fix or a fix to code raindrop actually ships; critical security fixes target
   48 hours. Do not chase Nightly between releases.
3. **Branches and tags.**
   - `upstream` is a linear import branch containing Firefox releases only,
     with no raindrop commits. Each import is tagged
     `firefox-<version>` (for example `firefox-160.0.1`), annotated with the
     upstream version, tarball URL and SHA-256.
   - `main` is the raindrop patch queue on top of the latest `upstream` tag. It
     is rebased, never merged, onto each new import.
   - Feature work happens on `topic/<slug>` branches and lands through a pull
     request.
   - Fork releases are tagged `v<raindrop>-fx<firefox>` (for example
     `v0.1.0-fx160.0.1`), annotated with the upstream tag and build provenance.
4. **Patch discipline.** One logical patch per commit, kept in the isolated
   `atproto/` module except for the thin integration points enumerated in ADR
   0001. Each commit message names the upstream files it touches and why.
   Patches are not squashed before release, so a rebase conflict maps back to
   exactly one patch.
5. **Build bootstrap.** The stock build in #10 uses `unofficial` branding; a
   raindrop release build selects
   `--with-branding=browser/branding/raindrop` once that branding directory
   exists (#11). CI builds the imported `firefox-<version>` tag plus the patch
   queue rather than a floating branch. `docs/BUILDING.md` records the exact
   upstream version and tarball hash used for the recorded build.

## Consequences

- The base is reproducible and signed, and every build can name its exact
  upstream version.
- The release channel only supports the current version. raindrop therefore
  has to ship within days of each Firefox release, or its users are running an
  unsupported engine. That is the main cost of this decision and the reason
  rebase/release automation is a Phase 3 requirement, not a polish item.
- Conflicts should be small and frequent rather than large and annual. The
  team will measure time spent per rebase and the files that conflict, not just
  assume the queue stays cheap.
- Nightly-only APIs are unavailable. If a feature needs one, it waits for the
  release channel; that is the correct trade for a browser people run.
- ESR remains a fallback, not the plan. If a rebase repeatedly takes more than
  a working day, or conflicts concentrate in the core files identified by ADR
  0001, this ADR should be reopened and ESR 153 considered as the primary base.

## Alternatives considered

- **mozilla-central / Nightly.** Rejected: daily churn, no supported release,
  and a rebase load that would consume the project before it has users.
- **ESR as the primary source.** Rejected for now: the engine is roughly
  sixteen weeks behind release, the annual line change is a large rebase, and
  the project's stated goal is to track upstream closely. It is retained above
  as the fallback if the two-week cadence proves unsustainable.
- **Tarballs without a Git import branch.** Rejected for daily work: no diff
  history between releases, which makes reviewing upstream changes and
  rebasing patches harder. Tarballs are still the verification authority.
- **Following the `release` branch head.** Rejected: it moves before a release
  is signed and supported, so it combines the cost of tracking without the
  guarantee of a release.

## References

- [Firefox release process](https://wiki.mozilla.org/Release_Management/Release_Process)
- [Firefox two-week cadence announcement](https://blog.mozilla.org/sumo/2026/08/19/firefox-new-release-cadence-and-what-to-expect)
- [Current Firefox trains](https://whattrainisitnow.com/)
- [Firefox source repository](https://github.com/mozilla-firefox/firefox)
- [Firefox release source tarballs](https://archive.mozilla.org/pub/firefox/releases/)
- [Firefox ESR release cycle](https://support.mozilla.org/en-US/kb/firefox-esr-release-cycle)
