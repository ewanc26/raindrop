# ADR 0011: visual-identity boundary

- **Status:** Accepted
- **Date:** 2026-11-22
- **Issue:** [#99](https://github.com/ewanc26/raindrop/issues/99)
- **Related:** [ARCHITECTURE.md](../ARCHITECTURE.md),
  [BRANDING.md](../BRANDING.md),
  [ADR 0001](0001-at-uri-scheme-registration.md),
  [ADR 0008](0008-at-document-customization.md)

## Context

raindrop is a Firefox fork, and its branding currently stops at the application
icon, the about dialog, brand strings and default bookmarks. The chrome, new tab
page and generated `at://` documents still use Firefox's visual system. Nothing
in the address bar, a tab or a record page tells a user that this browser
resolves `at://` directly.

Other Firefox forks differentiate mainly by rebuilding the chrome layout: Zen
uses a vertical sidebar and hidden title bar, Floorp adds configurable panels and
tab layouts, and LibreWolf mostly changes defaults. A layout fork is expensive to
rebase onto Firefox releases and is not what makes raindrop distinct. raindrop's
premise is a native `at://` scheme with direct PDS fetches and no gateway, so the
visual identity should concentrate on the surfaces where that premise becomes
visible.

## Decision

1. raindrop remains a Firefox fork with upstream chrome layout. No vertical-tab,
   sidebar or toolbar rewrite. Visual changes must remain a small, reviewable
   patch over upstream.
2. The visual language comes from the existing original artwork: night
   `#12294a`, sky `#38bdf8`, water `#0d9488`, highlight `#a5f3fc`, and the drop
   mark. No Mozilla, Firefox, AT Protocol or Bluesky marks. Colour is used for
   wayfinding, not as a page wallpaper.
3. The default appearance is a built-in static theme with light and dark
   variants, registered in `BuiltInThemeConfig.sys.mjs` and selected by a
   branding default for `extensions.activeThemeID`. A user's explicit theme
   choice remains a user pref and takes precedence. The theme uses the standard
   WebExtension theme pipeline rather than a parallel chrome CSS fork, so system
   light/dark, private windows and forced-colors keep working.
4. `at://` gets its own trusted-browser presentation: an address-bar identity
   state and provenance panel, a shared header on generated pages, a tab/history
   glyph and an `at://` entry point on the new tab page. These are tracked as
   #101, #102, #103 and #104.
5. First-run education is a local page (tracked as #105), never a remote
   Mozilla welcome URL.
6. Accessibility is part of the decision, not a follow-up: text contrast meets
   WCAG AA, UI and focus indicators meet 3:1, and every surface must remain
   usable in forced-colors and high-contrast modes and with reduced motion.
7. No remote assets, fonts, telemetry or endpoints are added for presentation.

## Consequences

- New profiles get a recognisably raindrop appearance without a layout fork;
  existing profiles with an explicit theme keep it.
- The theme patch is limited to `browser/themes/addons/raindrop/`,
  `BuiltInThemeConfig.sys.mjs`, the add-on jar and the branding prefs.
- The remaining visual work is isolated in trusted chrome and generated
  documents, where `at://` provenance already lives.
- A rebase may need to re-check theme schema changes and the built-in theme map,
  but does not need to merge a rewritten browser UI.
- The CID check is still presented as integrity only, not authorship. Visual
  identity must not overstate what verification proves.

## Implementation

The theme part of this decision is implemented in issue #100. The identity,
header, glyph, new tab and first-run work is tracked separately so each can be
reviewed against this boundary.
