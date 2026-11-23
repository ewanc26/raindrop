/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Blank endpoints and disabled updates keep raindrop from calling home as Firefox.

// raindrop's theme is the default for new profiles. An explicit theme choice is
// stored as a user value and takes precedence.
pref("extensions.activeThemeID", "raindrop@ewancroft.uk");

pref("startup.homepage_override_url", "");
pref("startup.homepage_welcome_url", "");
pref("startup.homepage_welcome_url.additional", "");

pref("app.update.enabled", false);
pref("app.update.url", "");
pref("app.update.auto", false);
pref("app.update.service.enabled", false);
pref("app.update.checkInstallTime", false);
pref("app.update.interval", 0);
pref("app.update.promptWaitTime", 0);
pref("app.update.url.manual", "");
pref("app.update.url.details", "");
pref("app.update.checkInstallTime.days", 0);
pref("app.update.badgeWaitTime", 0);

pref("toolkit.telemetry.enabled", false);
pref("datareporting.healthreport.uploadEnabled", false);

// Background services that transmit to Mozilla without user action.
pref("app.normandy.enabled", false);
pref("browser.topsites.contile.enabled", false);
pref("security.certerrors.mitm.priming.enabled", false);

// The add-on manager's periodic background check fetches a system-addon list
// from aus5.mozilla.org; raindrop has no Mozilla-hosted system add-ons.
pref("extensions.systemAddon.update.enabled", false);
pref("extensions.systemAddon.update.url", "");

// New-tab ads (MARS) would reach ads.mozilla.org on a normal new tab.
pref("browser.newtabpage.activity-stream.unifiedAds.spocs.enabled", false);
pref("browser.newtabpage.activity-stream.unifiedAds.tiles.enabled", false);
pref("browser.newtabpage.activity-stream.unifiedAds.endpoint", "");

pref("devtools.selfxss.count", 5);
