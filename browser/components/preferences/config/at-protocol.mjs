/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Preferences } from "chrome://global/content/preferences/Preferences.mjs";
import { SettingGroupManager } from "chrome://browser/content/preferences/config/SettingGroupManager.mjs";

const prefs = [
  "network.atprotocol.enabled",
  "network.atprotocol.record.showProvenance",
  "network.atprotocol.record.showRawJson",
];

Preferences.addAll(prefs.map(id => ({ id, type: "bool" })));
for (let id of prefs) {
  Preferences.addSetting({ id, pref: id });
}

SettingGroupManager.registerGroup("atProtocol", {
  l10nId: "at-protocol-settings",
  headingLevel: 2,
  items: [
    {
      id: "network.atprotocol.enabled",
      l10nId: "at-protocol-enabled",
    },
    {
      id: "network.atprotocol.record.showProvenance",
      l10nId: "at-protocol-show-provenance",
    },
    {
      id: "network.atprotocol.record.showRawJson",
      l10nId: "at-protocol-show-raw-json",
    },
  ],
});
