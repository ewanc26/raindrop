# Security Policy

Thank you for keeping raindrop safe, please report responsibly.

## Supported versions

raindrop is pre-release: only the `main` branch is supported. There are no
tagged releases of the browser yet, so there are no versions to backport fixes
to. Security-relevant fixes land on `main` directly.

## Reporting a vulnerability

- **Preferred:** use GitHub's private vulnerability reporting
  (Security tab -> "Report a vulnerability") so the details stay private.
- **Fallback:** email contact@ewancroft.uk with "raindrop security" in the
  subject.
- Please **do not** open a public issue or PR with exploit details, and give
  the project a chance to patch before disclosing publicly.

Handling is best-effort by a solo maintainer: there is no bounty program, no
SLA, and no promise of response times — but every report is read.

## Scope

- **In scope:** the `atproto/` tooling, the fork's own patches, CI, and
  anything in this repository, plus the design's trust model (e.g. a flaw that
  would make raindrop resolve `at://` URIs through an unintended party).
- **Out of scope here:** vulnerabilities in Firefox itself. raindrop builds on
  Mozilla's open-source code; report those to Mozilla at
  https://www.mozilla.org/security/ so everyone building on Firefox benefits.
