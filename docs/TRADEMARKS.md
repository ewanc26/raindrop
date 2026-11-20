# Trademarks and branding

raindrop is planned to be built from Mozilla Firefox open-source code. The
MPL-2.0 licence grants rights to the *code*; it grants **no rights in
Mozilla's trademarks, service marks, or logos**. This file records what that
means for raindrop, based on Mozilla's published trademark guidelines:
https://www.mozilla.org/foundation/trademarks/policy/

## Rules raindrop must follow

1. **Never ship under Firefox branding.** Modified builds may not carry
   Mozilla's names, logos, or trade dress in the product's user-facing name,
   icons, about screens, or installer. raindrop must always build with its own
   branding.
2. **Acceptable descriptive use.** The project may state *in words, not
   logos* that it is based on Mozilla's open-source technology (e.g. the
   README's description of raindrop as a Firefox fork), provided there is an
   equally prominent statement that it is not affiliated with Mozilla or
   endorsed by them — see [NOTICE](../NOTICE).
3. **No confusing names.** Mozilla's guidelines call out patterns like
   "[Something]-fox", "Moz-[Something]", and "[Something]-zilla" as names to
   avoid. "raindrop" follows none of them, and must not incorporate Mozilla
   marks into domains, social handles, or account names either
   (e.g. "Firefox raindrop", "@raindropfirefox").
4. **No Mozilla logos.** Don't use, remix, or recolour the Firefox logo, word
   mark, or other Mozilla brand assets anywhere in the project; raindrop gets
   its own icon and word mark, designed from scratch.

## How this is implemented in the build

Mozilla builds are aware of this: Firefox's own build system supports multiple
"branding" sets under `browser/branding/` — `official` (Release/Beta),
`aurora` (Developer Edition), `nightly`, and `unofficial` (used when no
other branding is specified, e.g. local developer builds):
https://firefox-source-docs.mozilla.org/browser/branding/docs/index.html

The fork's approach:

- raindrop owns a `browser/branding/raindrop/` directory (icons, product
  names, prefs) selected in the mozconfig via
  `ac_add_options --with-branding=browser/branding/raindrop`.
- Local developer builds intentionally fall back to `unofficial` branding,
  which already strips trademarked names and artwork — the correct behaviour
  for unbranded local builds of the fork.
- `brand.ftl` style brand strings must be filled with raindrop's own names
  (`-brand-short-name = raindrop`, and so on) so no Mozilla mark survives in
  the UI._Community tooling can generate a full drop-in branding directory from
  SVG sources when the project reaches that stage (see the
  FirefoxUX/firefox-brand generator).

## Practical checklist when the fork builds

- [ ] Binary, installer, and packaged app names contain no Mozilla mark
- [ ] Icons and about screens use raindrop-brand assets only
- [ ] `brand.ftl` brand strings set to raindrop names in all branding dirs
- [ ] Update checks and any Mozilla-default endpoints disabled or pointed at
      raindrop targets, so the build never phones home as "Firefox"
- [ ] This file and NOTICE kept in sync with the guidelines if Mozilla updates
      them

If in doubt, write to trademark-permissions@mozilla.org; *before* any first
public release is the cheapest time to check.
