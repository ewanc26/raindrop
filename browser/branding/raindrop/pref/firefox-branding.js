/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Blank endpoints and disabled updates keep raindrop from calling home as Firefox.

pref("startup.homepage_override_url", "");
pref("startup.homepage_welcome_url", "");
pref("startup.homepage_welcome_url.additional", "");

pref("app.update.enabled", false);
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

pref("devtools.selfxss.count", 5);
