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
bar, the same way `http://`, `mailto:`, and `file://` already are.

## Why the name?

The ecosystem built on the AT Protocol is known as the Atmosphere, and rain is
what an atmosphere is for. It is also the architecture in one image: a raindrop
leaves the cloud (the author's repository) and lands exactly where it is pointed,
with nothing in between. No gateway intermediates a raindrop. Each `at://` URI
is a drop, small, precisely addressed, and fetched directly from its own PDS
rather than routed through anyone else's sky.

## Why a whole browser, not an extension?

Extensions for `at://` exist today, and they demonstrate the problem: with no scheme
support underneath, the best they can do is catch the URI elsewhere and redirect it to
a web viewer. That keeps the gateway in the middle — the exact middleman raindrop
exists to remove. No WebExtension can fix this, because extensions fundamentally
cannot:

- **register a new URI scheme** — there is no API to make `at://` navigable, so an
  extension never even sees an `at://` address typed into the bar (the browser
  treats it as a search query), and existing extensions work by picking the URI up
  after the fact;
- **reach the URL parser** — the DID authority (`did:plc:...`, with its un-encoded
  colons) breaks WHATWG URL parsing, so correct handling has to live in the
  engine's URI machinery where raindrop can fix it, not in web-platform code;
- **fetch a PDS like the network stack can** — a page-context fetch needs CORS
  cooperation from every PDS operator, while the browser's fetcher, cache, and
  private-browsing integration are protocol-agnostic; or
- **put trust UI in chrome** — provenance ("which DID, which PDS, CID verified?")
  and honest failure states must be rendered by the browser itself, not by
  injectable content pretending to be it.

So an extension version of raindrop would have to compromise on the very goals
(native scheme, no gateway, verifiable provenance) that define the project. The
fuller design reasoning is in
[ARCHITECTURE.md](docs/ARCHITECTURE.md#why-in-the-browser-not-an-extension).

## Goals

- Native `at://` scheme handling in the address bar, bookmarks, and history.
- Built-in DID resolution (`did:plc`, `did:web`) and PDS record fetching, with no
  gateway in the middle.
- Repository summaries for authority-only addresses such as `at://ewancroft.uk`,
  and collection listings for `at://did/collection`, read directly from the PDS.
- Bundled post and profile views show blob CIDs and link to the resolved PDS's
  `getBlob` endpoint; no bytes are fetched until the link is followed. Reply
  parents, roots and quoted records become navigable `at://` links. Image blobs
  can be shown inline from the resolved PDS, with
  `network.atprotocol.record.showImages` to disable automatic image loads.
  Like, repost and follow records get compact views whose subjects become
  validated `at://` links (DID links for follows).
- Verifiable content: records are fetched from their PDS and checked against their
  CID, which proves the record is intact and self-consistent with the CID the PDS
  returned. This is integrity, not a repository signature or proof of authorship.
- Stay a *Firefox fork*, not a new browser: track upstream closely so the fork is
  maintainable.

## Non-goals

- Being a Bluesky client for everyday social use; full clients already do that well.
- A new engine or rendering stack.
- Custom per-app behaviour beyond rendering records for known lexicons.

## Status

The pristine, signed Firefox 161.0 release source is imported on the `upstream`
branch and tagged `firefox-161.0`; raindrop's work is the patch queue on `main`.
The `at://` scheme handler, DID and handle resolution, PDS record fetch, record
views and honest failure pages are implemented in `netwerk/protocol/atproto/`.
The tree builds locally, the module's GTests and xpcshell tests pass, and
v0.11.0 has been released on Firefox 160.0.1. The design and trademark sign-offs
are recorded. See the [roadmap](docs/ROADMAP.md) and
[releasing](docs/RELEASING.md) for current gates.

- [Roadmap](docs/ROADMAP.md) - phased milestones from scaffold to first release
- [Architecture](docs/ARCHITECTURE.md) - how an `at://` request flows through the browser
- [Building](docs/BUILDING.md) - upstream source provenance and local build setup
- [AT Protocol setup](docs/ATPROTO_SETUP.md) - local `at://` verification and smoke test
- [Branding](docs/BRANDING.md) - raindrop branding assets and trademark checklist
- [Releasing](docs/RELEASING.md) - release gates, tag naming and provenance

## Contributing

Issues and design feedback are welcome; see [CONTRIBUTING.md](CONTRIBUTING.md)
and [SUPPORT.md](SUPPORT.md).

## Support the project

raindrop is free and open source, and so is everything else its maintainer builds.
If the project is useful to you, your support keeps the servers running, projects
maintained, and new tools getting built:

- [Ko-fi](https://ko-fi.com/ewancroft) - one-time or monthly
- [GitHub Sponsors](https://github.com/sponsors/ewanc26) - one-time or monthly
- [Liberapay](https://liberapay.com/ewanc26) - recurring
- Everything in one place: [ewancroft.uk/support](https://ewancroft.uk/support)

Payments are handled securely by each platform; this project never sees your
payment details.

## Trademarks

Firefox and the Firefox logos, and Mozilla and the Mozilla logos, are trademarks
of the Mozilla Foundation. AT Protocol, atproto, Bluesky, and the Bluesky butterfly
logo are trademarks of Bluesky Social PBC. raindrop is an independent project,
not affiliated with or endorsed by Mozilla or Bluesky; it uses Mozilla's
open-source code under the MPL and interoperates with the AT Protocol, but uses
no one's trademarks as branding. See [NOTICE](NOTICE) and
[docs/TRADEMARKS.md](docs/TRADEMARKS.md).

## Licence

raindrop is intended to be licensed under the Mozilla Public License 2.0, keeping it
compatible with the Firefox code it will be built from. See [LICENSE](LICENSE).
