# ADR 0001: minimal patch surface for `at://` scheme registration

- **Status:** Draft
- **Date:** 2026-11-20
- **Issue:** [#12](https://github.com/ewanc26/raindrop/issues/12)
- **Related:** [ARCHITECTURE.md](../ARCHITECTURE.md), [upstream-tracking decision (#8)](https://github.com/ewanc26/raindrop/issues/8), [ROADMAP Phase 0](../ROADMAP.md)

## Context

raindrop must make `at://` navigable in the address bar, bookmarks and history.
Two Gecko facts shape the patch:

1. **Registration is manifest-driven in current Firefox.** Static protocol
   handlers are generated from `components.conf` manifests by
   [`xpcom/components/gen_static_components.py`](https://searchfox.org/firefox-main/source/xpcom/components/gen_static_components.py);
   the generated record shape lives in
   [`xpcom/components/StaticComponents.h`](https://searchfox.org/firefox-main/source/xpcom/components/StaticComponents.h).
   Core schemes are declared in [`netwerk/build/components.conf`](https://searchfox.org/firefox-main/source/netwerk/build/components.conf)
   and the handler contract convention is
   `@mozilla.org/network/protocol;1?name=<scheme>` in
   [`netwerk/base/nsIProtocolHandler.idl`](https://searchfox.org/firefox-main/source/netwerk/base/nsIProtocolHandler.idl).
   At runtime [`netwerk/base/nsIOService.cpp`](https://searchfox.org/firefox-main/source/netwerk/base/nsIOService.cpp)
   lowercases the scheme and checks the static table before runtime handlers.
   Generated files must never be edited by hand.
2. **The authority is not a host:port.** An AT-URI authority such as
   `did:plc:abc...` contains unencoded colons; the IANA registration and the
   IETF `at` URI draft note that this precludes WHATWG URL compatibility.
   [`netwerk/base/nsURLParsers.cpp`](https://searchfox.org/firefox-main/source/netwerk/base/nsURLParsers.cpp)
   (`nsAuthURLParser::ParseAuthority`, `ParseServerInfo`) and
   [`netwerk/base/nsStandardURL.cpp`](https://searchfox.org/firefox-main/source/netwerk/base/nsStandardURL.cpp)
   (`SetHostPort`, `SetHost`) interpret colon-containing authorities as
   host/port pairs or reject them. Unknown schemes otherwise fall back to
   [`netwerk/base/DefaultURI.cpp`](https://searchfox.org/firefox-main/source/netwerk/base/DefaultURI.cpp),
   which delegates to the Rust `url` crate (see
   [`netwerk/base/mozurl/Cargo.toml`](https://searchfox.org/firefox-main/source/netwerk/base/mozurl/Cargo.toml));
   that fallback is controlled by `network.url.useDefaultURI` and
   `network.url.simple_uri_unknown_schemes_enabled` in
   [`modules/libpref/init/StaticPrefList.yaml`](https://searchfox.org/firefox-main/source/modules/libpref/init/StaticPrefList.yaml)
   and the branches in
   [`netwerk/base/nsNetUtil.cpp`](https://searchfox.org/firefox-main/source/netwerk/base/nsNetUtil.cpp).

The address bar is already on our side:
[`docshell/base/URIFixup.sys.mjs`](https://searchfox.org/firefox-main/source/docshell/base/URIFixup.sys.mjs)
treats a scheme as internally handled when
`Services.io.getProtocolHandler(scheme) != defaultProtocolHandler`, and
[`browser/components/urlbar/UrlbarUtils.sys.mjs`](https://searchfox.org/firefox-main/source/browser/components/urlbar/UrlbarUtils.sys.mjs)
maps that fixup result into URL (not search) behaviour. Registering a handler
should therefore be enough to stop `at://did:plc:...` being treated as a search
query, without touching front-end code. This is confirmed by the
`at_uri_is_classified_as_a_url_not_a_search` xpcshell test in
`netwerk/protocol/atproto/tests/test_at_protocol.js`, which calls
`Services.uriFixup.getFixupURIInfo` with keyword lookup allowed for both DID and
handle authorities and asserts a URL fixup with no `keywordAsSent`.

The guiding constraint from the roadmap and ADR 0002 is to keep the rebase cost
low: new files where possible, one logical patch per change, and no edits to
widely changing shared files.

## Decision

**Register `at` from a new, isolated module and implement AT-URI parsing inside
that module. Modify exactly one existing file to wire the module into the
build.**

1. Add `netwerk/protocol/atproto/` containing an `nsIProtocolHandler`
   implementation for scheme `at`, an `nsIURI` implementation (or helper)
   that treats the DID/handle authority as an **opaque string**, a module-local
   `components.conf` registering
   `@mozilla.org/network/protocol;1?name=at` via `protocol_config`, a
   `moz.build`, and tests.
2. Modify only [`netwerk/protocol/moz.build`](https://searchfox.org/firefox-main/source/netwerk/protocol/moz.build)
   to add the new subdirectory, following the existing `DIRS += [...]` pattern.
3. Do **not** modify `NS_NewURI`, `nsStandardURL`, `nsURLParsers`,
   `URIFixup.sys.mjs`, `UrlbarUtils.sys.mjs`, `StaticPrefList.yaml`, or browser
   front-end code unless a runtime test demonstrates a concrete failure that
   cannot be solved in the module.
4. Validate in Gecko tests that `Services.io.getProtocolHandler("at")` returns
   our handler, that `at://did:plc:abc.../app.bsky.feed.post/3l...` round-trips
   exactly through `nsIURI.spec`, and that typing it in the address bar is
   classified as a URL. All three are covered by
   `netwerk/protocol/atproto/tests/test_at_protocol.js` (`protocol_handler_is_registered`,
   `did_authority_round_trips`, `at_uri_is_classified_as_a_url_not_a_search`).

## Patch surface

### Files to add

| File | Purpose |
| --- | --- |
| `netwerk/protocol/atproto/components.conf` | `protocol_config` entry for scheme `at` (exact flags to be chosen against `nsIProtocolHandler.idl`) |
| `netwerk/protocol/atproto/AtProtocolHandler.h` / `.cpp` | `nsIProtocolHandler` for `at`; creates the record-fetch channel |
| `netwerk/protocol/atproto/AtURI.h` / `.cpp` | `nsIURI` implementation with AT-URI-specific, spec-conformant parsing |
| `netwerk/protocol/atproto/moz.build` | Sources, exports, `XPCOM_MANIFESTS`, tests |
| `netwerk/protocol/atproto/tests/...` | Protocol lookup, URI round-trip, malformed-URI and fixup tests |

### Files to modify

| File | Change | Rebase risk |
| --- | --- | --- |
| `netwerk/protocol/moz.build` | Add `atproto` to `DIRS += [...]` | Low |

### Files deliberately not modified

| File | Reason |
| --- | --- |
| `netwerk/build/components.conf` | A module-local manifest is sufficient; there is precedent in comm-central for product-owned `components.conf` files (e.g. `mailnews/news/src/components.conf`, `calendar/base/src/components.conf`), and editing the central manifest adds avoidable conflict surface |
| `netwerk/base/nsNetUtil.cpp`, `netwerk/base/nsStandardURL.cpp`, `netwerk/base/nsURLParsers.cpp` | Shared URI machinery; changing it affects every scheme and carries correctness and security risk |
| `docshell/base/URIFixup.sys.mjs`, `browser/components/urlbar/UrlbarUtils.sys.mjs` | Handler registration already drives classification; UX changes are a separate decision |
| `modules/libpref/init/StaticPrefList.yaml`, browser front-end files | Broadly patched by forks; high conflict probability for no benefit at this stage |

## Rebase risk assessment

- **New module files:** low risk. They are additions; upstream changes cannot
  conflict with them, and the only integration point is a one-line `DIRS`
  entry.
- **`netwerk/protocol/moz.build`:** low–medium risk. It is a small, stable file,
  but upstream can add or reorder subdirectories.
- **Generated files (`StaticComponents.cpp`, `StaticComponentData.h`,
  `components.rs`):** never touched; regenerated by the build.
- The riskiest area is not rebasing but correctness: protocol flags must be
  chosen carefully, because
  [`nsIProtocolHandler.idl`](https://searchfox.org/firefox-main/source/netwerk/base/nsIProtocolHandler.idl)
  constrains valid flag combinations and marks `ORIGIN_IS_FULL_SPEC` as
  Thunderbird/SeaMonkey-only. A wrong flag choice can silently change origin or
  security behaviour, so flag selection is a review gate, not an implementation
  detail.

## Alternatives considered

- **Register `at` directly in `netwerk/build/components.conf`.** Fewer new
  files, but edits a central manifest that upstream changes regularly. Rejected
  in favour of a module-local manifest.
- **Patch `NS_NewURI` / `nsStandardURL` / `nsURLParsers` to special-case DIDs.**
  Rejected: very high rebase risk and shared-parser correctness risk. The DID
  authority must be parsed by AT-URI code, not squeezed into host/port logic.
- **Patch URLbar fixup or prefs.** Rejected: current fixup already keys off
  registered protocol handlers
  ([`URIFixup.sys.mjs`](https://searchfox.org/firefox-main/source/docshell/base/URIFixup.sys.mjs)).
  Revisit only if runtime tests fail.
- **Ship a WebExtension.** Rejected in [ARCHITECTURE.md](../ARCHITECTURE.md#why-in-the-browser-not-an-extension):
  extensions cannot register a scheme or replace the URL parser.

## References

- [`xpcom/components/gen_static_components.py`](https://searchfox.org/firefox-main/source/xpcom/components/gen_static_components.py)
- [`xpcom/components/StaticComponents.h`](https://searchfox.org/firefox-main/source/xpcom/components/StaticComponents.h)
- [`netwerk/build/components.conf`](https://searchfox.org/firefox-main/source/netwerk/build/components.conf)
- [`netwerk/base/nsIProtocolHandler.idl`](https://searchfox.org/firefox-main/source/netwerk/base/nsIProtocolHandler.idl)
- [`netwerk/base/nsIOService.cpp`](https://searchfox.org/firefox-main/source/netwerk/base/nsIOService.cpp)
- [`netwerk/base/nsNetUtil.cpp`](https://searchfox.org/firefox-main/source/netwerk/base/nsNetUtil.cpp)
- [`netwerk/base/nsStandardURL.cpp`](https://searchfox.org/firefox-main/source/netwerk/base/nsStandardURL.cpp)
- [`netwerk/base/nsURLParsers.cpp`](https://searchfox.org/firefox-main/source/netwerk/base/nsURLParsers.cpp)
- [`netwerk/base/DefaultURI.cpp`](https://searchfox.org/firefox-main/source/netwerk/base/DefaultURI.cpp)
- [`docshell/base/URIFixup.sys.mjs`](https://searchfox.org/firefox-main/source/docshell/base/URIFixup.sys.mjs)
- [`browser/components/urlbar/UrlbarUtils.sys.mjs`](https://searchfox.org/firefox-main/source/browser/components/urlbar/UrlbarUtils.sys.mjs)
- [Firefox Source Docs: Defining XPCOM components](https://firefox-source-docs.mozilla.org/build/buildsystem/defining-xpcom-components.html)
- [LibreWolf patch queue](https://librewolf.dev/librewolf/source/src/branch/main/patches) and [Tor Browser development overview](https://www.pierov.org/2023/04/13/tor-browser-development) (fork-maintenance precedent only; no directly reusable protocol-handler patch was found)
