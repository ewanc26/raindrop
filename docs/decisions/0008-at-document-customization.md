# ADR 0008: at:// document customization

- **Status:** Accepted
- **Date:** 2026-11-22
- **Issue:** [#93](https://github.com/ewanc26/raindrop/issues/93)
- **Related:** [ARCHITECTURE.md](../ARCHITECTURE.md), [ADR 0001](0001-at-uri-scheme-registration.md)

## Context

Raindrop generates the HTML for every `at://` page inside the browser. Record,
repository, collection and error views duplicated their own `<style>` blocks with
fixed colours, text size, margins and width. The only display choices offered to
the user were provenance, raw JSON and inline images.

That left two gaps. Users whose system uses dark mode still received a white page
with dark text, and there was no way to adjust legibility or density. The
duplicated CSS also meant a change to one view could not be applied consistently
to the others.

The generated pages are local documents with an opaque origin. Their CSP allows
inline styles and no scripts or remote stylesheets, so presentation can be
customized without giving content any new capability.

## Decision

1. A single `AppendDocumentStyle` function in `netwerk/protocol/atproto/`
   generates the style block for all four views. Each view passes a
   `ViewStyleOptions` value; the resolver loads it from preferences once per
   response.
2. Five string preferences control presentation, all with safe defaults:

   | Preference | Default | Choices |
   | --- | --- | --- |
   | `network.atprotocol.view.theme` | `system` | `system`, `light`, `dark`, `high-contrast` |
   | `network.atprotocol.view.textSize` | `medium` | `small`, `medium`, `large`, `extra-large` |
   | `network.atprotocol.view.width` | `standard` | `narrow`, `standard`, `wide`, `full` |
   | `network.atprotocol.view.accent` | `blue` | `blue`, `green`, `purple`, `amber`, `slate` |
   | `network.atprotocol.view.density` | `comfortable` | `comfortable`, `compact` |
   | `network.atprotocol.collection.pageSize` | `50` | `10`, `25`, `50`, `100` |

3. Values are selected from fixed sets. An unknown value maps to the default
   rather than being copied into CSS. No preference value is interpolated into
   the document as raw text.
4. The theme choices are `system` (follow `prefers-color-scheme`), explicit
   light, explicit dark and a white-on-black high-contrast option with underlined
   links. Accent colours have separate light and dark variants so links stay
   legible in both modes.
5. `network.atprotocol.collection.pageSize` sets the `limit` on
   `com.atproto.repo.listRecords`. The value is one of four fixed strings and the
   URL builder clamps it to 10–100 as a second guard. Other identity, PDS and
   blob requests are unaffected.
6. All choices are local. The appearance settings change only generated `at://`
   documents; the page-size setting changes how many records one PDS request
   asks for. The settings text says so.
7. The security headers, security meta tags, CSP, referrer policy, body limits,
   cancellation and HTML escaping contracts are unchanged.

## Consequences

- A clean profile gets a readable page in both light and dark system themes and
  2560 valid combinations of the six settings.
- Renderers stay pure functions: tests pass an explicit `ViewStyleOptions` value
  and do not depend on the preference service.
- Adding a new theme or accent requires one enum value, one lookup branch and one
  localization entry; no view needs editing.
- The settings are cosmetic. They do not change resolution authority, add a
  gateway or make any additional network request.
