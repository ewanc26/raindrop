/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Preferences } from "chrome://global/content/preferences/Preferences.mjs";
import { SettingGroupManager } from "chrome://browser/content/preferences/config/SettingGroupManager.mjs";

const boolPrefs = [
  "network.atprotocol.enabled",
  "network.atprotocol.record.showProvenance",
  "network.atprotocol.record.showRawJson",
  "network.atprotocol.record.showImages",
];

const stringPrefs = [
  "network.atprotocol.view.theme",
  "network.atprotocol.view.textSize",
  "network.atprotocol.view.width",
  "network.atprotocol.view.accent",
  "network.atprotocol.view.density",
  "network.atprotocol.collection.pageSize",
];

Preferences.addAll([
  ...boolPrefs.map(id => ({ id, type: "bool" })),
  ...stringPrefs.map(id => ({ id, type: "string" })),
]);
for (let id of [...boolPrefs, ...stringPrefs]) {
  Preferences.addSetting({ id, pref: id });
}

const options = values =>
  values.map(value => ({
    value,
    l10nId: `at-protocol-option-${value}`,
  }));

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
    {
      id: "network.atprotocol.record.showImages",
      l10nId: "at-protocol-show-images",
    },
    {
      id: "network.atprotocol.view.theme",
      l10nId: "at-protocol-view-theme",
      control: "moz-select",
      options: options(["system", "light", "dark", "high-contrast"]),
    },
    {
      id: "network.atprotocol.view.textSize",
      l10nId: "at-protocol-view-text-size",
      control: "moz-select",
      options: options(["small", "medium", "large", "extra-large"]),
    },
    {
      id: "network.atprotocol.view.width",
      l10nId: "at-protocol-view-width",
      control: "moz-select",
      options: options(["narrow", "standard", "wide", "full"]),
    },
    {
      id: "network.atprotocol.view.accent",
      l10nId: "at-protocol-view-accent",
      control: "moz-select",
      options: options(["blue", "green", "purple", "amber", "slate"]),
    },
    {
      id: "network.atprotocol.view.density",
      l10nId: "at-protocol-view-density",
      control: "moz-select",
      options: options(["comfortable", "compact"]),
    },
    {
      id: "network.atprotocol.collection.pageSize",
      l10nId: "at-protocol-page-size",
      control: "moz-select",
      options: options(["10", "25", "50", "100"]),
    },
  ],
});
