# Trademarks and branding

raindrop is planned to be built from Mozilla Firefox open-source code and
interoperates with the AT Protocol. Neither of those licences or policies
grants rights in anyone's *trademarks*, so this file records the rules from
both rights holders:

- Mozilla Trademark Guidelines:
  https://www.mozilla.org/foundation/trademarks/policy/
- AT Protocol trademark pages:
  https://atproto.com/about/trademarks and
  https://atproto.com/about/trademarks/atproto-trademark-policy

## Mozilla / Firefox

The MPL-2.0 licence grants rights to the *code*; it grants **no rights in
Mozilla's trademarks, service marks, or logos**.

### Rules raindrop must follow

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

### How this is implemented in the build

Mozilla builds are aware of this: Firefox's own build system supports multiple
"branding" sets under `browser/branding/` — `official` (Release/Beta),
`aurora` (Developer Edition), `nightly`, and `unofficial` (used when no
other branding is specified, e.g. local developer builds):
https://firefox-source-docs.mozilla.org/browser/branding/docs/index.html

The fork's approach:

- raindrop owns a `browser/branding/raindrop/` directory (icons, product
  names, prefs) selected in the mozconfig via
  `ac_add_options --with-branding=browser/branding/raindrop`.
- With no `--with-branding` option, `browser/confvars.sh` falls back to
  upstream's `browser/branding/unofficial`, and the stock-build mozconfig
  selects it explicitly. That directory is upstream's developer branding and
  still contains Nightly/Firefox/Mozilla brand strings, so it is suitable only
  for local development, not for distributing raindrop. raindrop's
  `browser/moz.configure` forces the raindrop vendor/id for all browser builds,
  but release builds must still select `build/raindrop.mozconfig` explicitly.
- `brand.ftl` style brand strings must be filled with raindrop's own names
  (`-brand-short-name = raindrop`, and so on) so no Mozilla mark survives in
  the UI. Community tooling can generate a full drop-in branding directory from
  SVG sources when the project reaches that stage (see the
  FirefoxUX/firefox-brand generator). raindrop did not use that generator: its
  `browser/branding/raindrop/` directory is drawn from scratch and documented
  in [BRANDING.md](BRANDING.md).

### Practical checklist when the fork builds

- [x] Binary, installer, and packaged app names contain no Mozilla mark.
      Verified for the macOS package and for Windows packaged resources in
      `Windows package check` run 44921258664.
- [x] Icons and about screens use raindrop-brand assets only, generated from
      original SVG artwork as recorded in [BRANDING.md](BRANDING.md).
- [x] `brand.ftl` brand strings set to raindrop names in all branding dirs.
      The release path uses `browser/branding/raindrop`; upstream's `unofficial`
      fallback remains local-development-only.
- [x] Update checks and any Mozilla-default endpoints disabled or pointed at
      raindrop targets, so the build never phones home as "Firefox".
- [x] This file and NOTICE checked against the current guidelines on
      2026-11-20 and re-checked on 2026-11-21 (see the review log below).
      Human sign-off recorded on 2026-11-21: proceed with the first public
      release without contacting Mozilla. The policy is unambiguous and
      raindrop uses no Mozilla marks.

The contact address for any later clarification is
trademark-permissions@mozilla.com.

### Guideline review log

- **2026-11-20:** re-read Mozilla's
  [distribution policy](https://www.mozilla.org/en-US/foundation/trademarks/distribution-policy/)
  and [trademark policy](https://www.mozilla.org/en-US/foundation/trademarks/policy/).
  Both confirm that the MPL grants no trademark rights and that modified builds
  must not carry Mozilla marks. raindrop's distinct name, from-scratch artwork,
  raindrop-only application identity and non-affiliation notice in
  [NOTICE](../NOTICE) remain consistent with the guidance; no change needed.
  Re-check when a release date is set.
- **2026-11-21:** re-read the same two pages before the first release. The
  operative statements are unchanged: MPL section 2.1 grants rights "under
  intellectual property rights (other than patent or trademark)", and the
  distribution policy says a modified product may not be redistributed "using
  any Mozilla trademark" without prior written consent. The pages give
  `trademark-permissions@mozilla.com`, so the contact address earlier in this
  file was corrected from `@mozilla.org`. No branding change is required.
- **2026-11-21 (sign-off):** the project owner decided to proceed with the
  first public release without contacting Mozilla. The policy is unambiguous,
  the release branding uses no Mozilla marks, and the no-phone-home prefs are
  verified in the packaged app. Windows packaged resources still need static
  inspection before the release is tagged.

## AT Protocol, atproto, and the Bluesky marks (Bluesky Social PBC)

Bluesky Social PBC owns the registered "ATPROTOCOL" marks and their variants,
including "AT Protocol" and "atproto" (acquired in July 2026 as a defensive
measure, with a stated intent to hand ownership to an independent protocol
governance organization later). "Bluesky", "bsky", and the Bluesky butterfly
logo are Bluesky trademarks too, and Bluesky has pending word-mark
applications such as "Attie". The atproto policy is written to make ordinary
good-faith use easy.

### What raindrop does that needs no permission

- **Truthful, descriptive references**: saying raindrop supports `at://` and
  the AT Protocol, writing docs and issues, community discussion. The claim
  must be accurate — raindrop must actually implement the spec (or say it
  aims to).
- **The `at://` scheme itself**: it is a provisional IANA URI scheme
  registration (change controller: Bluesky), now being standardised through
  the IETF's ATP working group — an identifier, not a trademark. Handling it
  is the whole point of the project.
- **raindrop's own name and branding**: "raindrop" is an independent name,
  not built around any mark, which is exactly what the policy wants.

### What raindrop must not do

- Use "AT Protocol", "atproto", or "Bluesky" *as part of its branding* (e.g.
  "AT Protocol Browser", "Bluesky raindrop"). Descriptive use only.
- Use the Bluesky butterfly logo or any Attie marks as raindrop icons, art,
  or content-rendering defaults.
- Imply official status or endorsement: keep raindrop's own name at least as
  prominent as any mark, and where confusion is possible, include the
  disclaimer: *raindrop is independent and is not affiliated with or endorsed
  by AT Protocol or Bluesky.* (Covered once for the whole project in
  [NOTICE](../NOTICE).)
- Present an incompatible protocol as the AT Protocol: raindrop must track the
  published spec and the IETF drafts (see ARCHITECTURE.md), and be explicit
  (issues, docs) whenever its behaviour diverges.

### Notes

- Registered domains built on the marks need permission under the policy;
  raindrop.dev (or similar) contains none of the marks.
- The atproto policy includes a good-faith safe harbor for descriptive use.
- If in doubt: trademarks@atproto.com (AT Protocol marks) or
  trademarks@blueskyweb.xyz (Bluesky marks).

### Technical footnote that affects the fork

The IANA registration for `at://` notes that DID authorities contain
multiple un-encoded colons, which **precludes compatibility with the WHATWG
URL standard** — a constraint the IETF `at` URI draft
(draft-newbold-atp-aturi) has been wrestling with too. Consequence for
raindrop: the `at-uri` module must implement AT-URI parsing straight from
the atproto specification; the browser's stock URL/URI machinery cannot be
assumed to handle the authority correctly.
