/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const { AddonManager } = ChromeUtils.importESModule(
  "resource://gre/modules/AddonManager.sys.mjs"
);
const { BuiltInThemes } = ChromeUtils.importESModule(
  "resource:///modules/BuiltInThemes.sys.mjs"
);

const THEME_ID = "raindrop@ewancroft.uk";

add_task(async function test_raindrop_theme_is_default() {
  Assert.equal(
    Services.prefs
      .getDefaultBranch("")
      .getCharPref("extensions.activeThemeID", ""),
    THEME_ID,
    "raindrop's theme is the default for new profiles"
  );
});

add_task(async function test_raindrop_theme_installs() {
  const config = BuiltInThemes.builtInThemeMap.get(THEME_ID);
  Assert.ok(config, "raindrop theme is registered");

  await AddonManager.maybeInstallBuiltinAddon(
    THEME_ID,
    config.version,
    config.path
  );
  const addon = await AddonManager.getAddonByID(THEME_ID);
  Assert.ok(addon, "raindrop theme installs from its manifest");
  Assert.equal(addon.type, "theme");
  Assert.equal(addon.name, "raindrop");

  const preview = BuiltInThemes.previewForBuiltInThemeId(THEME_ID);
  Assert.ok(preview.endsWith("/preview.svg"));
  const response = await fetch(preview);
  Assert.ok(response.ok, "raindrop theme preview is packaged");
});
