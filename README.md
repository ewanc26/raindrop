# raindrop

> a Firefox fork that has first-class support for the AT Protocol and its URI schema

## Why

The AT Protocol addresses every record on the open social web with an `at://` URI:
`at://did:plc:example/app.bsky.feed.post/3l...`. No mainstream browser understands
that scheme, so today the only way to reach such content is an HTTP gateway or an
app-specific web client, which reintroduces a trusted third party into a protocol
designed to not need one.

raindrop's premise is that the browser should speak the protocol itself. Instead of
teaching users about gateways, `at://` becomes a first-class citizen of the address
bar, the same way `http://`, `mailto://`, and `file://` already are.

## Goals

- Native `at://` scheme handling in the address bar, bookmarks, and history.
- Built-in DID resolution (`did:plc`, `did:web`) and PDS record fetching, with no
  gateway in the middle.
- Verifiable content: records are fetched from their PDS and can be checked against
  their CID, so what you see is what the author signed.
- Stay a *Firefox fork*, not a new browser: track upstream closely so the fork is
  maintainable.

## Non-goals

- Being a Bluesky client for everyday social use; full clients already do that well.
- A new engine or rendering stack.
- Custom per-app behaviour beyond rendering records for known lexicons.

## Status

Design / scaffold stage. There is no browser code in this repository yet; the
first milestone is to get the design and build pipeline right before touching
`mozilla-central`.

- [Roadmap](docs/ROADMAP.md) - phased milestones from scaffold to first release
- [Architecture](docs/ARCHITECTURE.md) - how an `at://` request flows through the browser

## Contributing

Issues and design feedback are very welcome while the project is at this stage; see
[CONTRIBUTING.md](CONTRIBUTING.md) and [SUPPORT.md](SUPPORT.md).

## Trademarks

Firefox and the Firefox logos, and Mozilla and the Mozilla logos, are trademarks
of the Mozilla Foundation. raindrop is an independent project, not affiliated
with or endorsed by Mozilla; it uses Mozilla's open-source code under the MPL but
none of its trademarks. See [NOTICE](NOTICE) and [docs/TRADEMARKS.md](docs/TRADEMARKS.md).

## Licence

raindrop is intended to be licensed under the Mozilla Public License 2.0, keeping it
compatible with the Firefox code it will be built from. See [LICENSE](LICENSE).
