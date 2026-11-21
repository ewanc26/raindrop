# Contributing

Thank you for wanting to help. raindrop has shipped releases, but the native
`at://` integration is still young: the most valuable contributions are browser
integration tests, security hardening, and careful upstream-rebase knowledge.

## Ways to help

- Read [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) and open issues challenging
  any part of the trust and failure model.
- Verify a clean build with [docs/ATPROTO_SETUP.md](docs/ATPROTO_SETUP.md) and
  report anything that fails or needs a clearer step.
- Pick up an open integration, security or setup issue from the
  [roadmap](docs/ROADMAP.md).

## Ground rules

- Discuss designs in issues before large PRs.
- Keep patches in the isolated `atproto/` module where possible; anything that
  touches stock Firefox code should be flagged in the PR description.
- Run the verifier and the AT test suites from
  [docs/ATPROTO_SETUP.md](docs/ATPROTO_SETUP.md) before opening a PR.
- Be kind. This project builds on open protocol work; treat everyone the way the
  Atmosphere community treats newcomers.
