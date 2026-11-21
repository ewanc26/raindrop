/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/ */

"use strict";

const ENABLED_PREF = "network.atprotocol.enabled";
const PROVENANCE_PREF = "network.atprotocol.record.showProvenance";
const RAW_JSON_PREF = "network.atprotocol.record.showRawJson";
const IMAGES_PREF = "network.atprotocol.record.showImages";

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
  ]) {
    ok(getSettingControl(pref, win), `${pref} setting-control exists`);
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
