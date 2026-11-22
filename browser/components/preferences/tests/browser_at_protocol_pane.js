/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/ */

"use strict";

const ENABLED_PREF = "network.atprotocol.enabled";
const PROVENANCE_PREF = "network.atprotocol.record.showProvenance";
const RAW_JSON_PREF = "network.atprotocol.record.showRawJson";
const IMAGES_PREF = "network.atprotocol.record.showImages";
const VIEW_PREFS = {
  "network.atprotocol.view.theme": ["system", "light", "dark", "high-contrast"],
  "network.atprotocol.view.textSize": [
    "small",
    "medium",
    "large",
    "extra-large",
  ],
  "network.atprotocol.view.width": ["narrow", "standard", "wide", "full"],
  "network.atprotocol.view.accent": [
    "blue",
    "green",
    "purple",
    "amber",
    "slate",
  ],
  "network.atprotocol.view.density": ["comfortable", "compact"],
  "network.atprotocol.collection.pageSize": ["10", "25", "50", "100"],
};

add_task(async function test_at_protocol_pane_loads() {
  let tab = await openPrefsTab("atProtocol");
  let win = tab.linkedBrowser.contentWindow;
  let doc = win.document;

  is_element_visible(
    doc.getElementById("category-at-protocol"),
    "AT Protocol category is visible"
  );

  await BrowserTestUtils.waitForMutationCondition(
    doc.getElementById("mainPrefPane"),
    { childList: true, subtree: true },
    () => doc.querySelector('setting-group[groupid="atProtocol"]')
  );

  let group = doc.querySelector('setting-group[groupid="atProtocol"]');
  is_element_visible(group, "AT Protocol setting group is visible");

  for (let pref of [
    ENABLED_PREF,
    PROVENANCE_PREF,
    RAW_JSON_PREF,
    IMAGES_PREF,
    ...Object.keys(VIEW_PREFS),
  ]) {
    ok(getSettingControl(pref, win), `${pref} setting-control exists`);
  }

  await BrowserTestUtils.removeTab(tab);
});

add_task(async function test_at_protocol_view_controls_offer_all_options() {
  let tab = await openPrefsTab("atProtocol");
  let win = tab.linkedBrowser.contentWindow;

  for (let [pref, expected] of Object.entries(VIEW_PREFS)) {
    let control = getSettingControl(pref, win);
    ok(control, `${pref} setting-control exists`);
    await control.updateComplete;

    let select = control.controlEl;
    is(select.localName, "moz-select", `${pref} uses a select control`);
    await BrowserTestUtils.waitForMutationCondition(
      select,
      { childList: true, subtree: true },
      () => select.inputEl?.options.length > 0
    );

    let values = Array.from(select.inputEl.options).map(option => option.value);
    for (let value of expected) {
      ok(values.includes(value), `${pref} offers ${value}`);
    }
    is(select.value, Services.prefs.getCharPref(pref), `${pref} follows pref`);
  }

  await BrowserTestUtils.removeTab(tab);
});

add_task(async function test_at_protocol_nav_button() {
  let tab = await openPrefsTab("");
  let doc = tab.linkedBrowser.contentDocument;
  let navButton = doc.getElementById("category-at-protocol");

  await TestUtils.waitForCondition(
    () => navButton?.buttonEl,
    "Wait for AT Protocol nav button to render"
  );

  let paneLoaded = waitForPaneChange("atProtocol");
  synthesizeClick(navButton);
  await paneLoaded;

  ok(
    doc.querySelector('setting-group[groupid="atProtocol"]'),
    "AT Protocol pane loaded after clicking its nav button"
  );

  await BrowserTestUtils.removeTab(tab);
});

add_task(async function test_at_protocol_controls_update_prefs() {
  let tab = await openPrefsTab("atProtocol");
  let win = tab.linkedBrowser.contentWindow;

  for (let pref of [
    ENABLED_PREF,
    PROVENANCE_PREF,
    RAW_JSON_PREF,
    IMAGES_PREF,
  ]) {
    let original = Services.prefs.getBoolPref(pref);
    let control = getSettingControl(pref, win);
    ok(control, `${pref} setting-control exists`);
    await control.updateComplete;

    let changed = waitForSettingControlChange(control);
    synthesizeClick(control.controlEl);
    await changed;

    is(
      Services.prefs.getBoolPref(pref),
      !original,
      `${pref} changes when its control is clicked`
    );
    Services.prefs.setBoolPref(pref, original);
  }

  await BrowserTestUtils.removeTab(tab);
});
