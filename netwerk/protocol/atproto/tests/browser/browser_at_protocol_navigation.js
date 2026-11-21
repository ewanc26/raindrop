/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const AT_URI = "at://did:web:invalid.example/app.bsky.feed.post/123";
const PAGE_URL =
  "https://example.com/browser/netwerk/protocol/atproto/tests/browser/at_link.html";

function readClipboardText() {
  const transferable = Cc["@mozilla.org/widget/transferable;1"].createInstance(
    Ci.nsITransferable
  );
  transferable.init(null);
  transferable.addDataFlavor("text/plain");
  Services.clipboard.getData(transferable, Ci.nsIClipboard.kGlobalClipboard);
  const data = {};
  transferable.getTransferData("text/plain", data);
  return data.value?.QueryInterface(Ci.nsISupportsString)?.data ?? "";
}

add_setup(async function () {
  await SpecialPowers.pushPrefEnv({
    set: [["network.dns.disabled", true]],
  });
  registerCleanupFunction(() =>
    Services.clipboard.emptyClipboard(Ci.nsIClipboard.kGlobalClipboard)
  );
});

add_task(async function fetch_rejects_at_uris() {
  await BrowserTestUtils.withNewTab(PAGE_URL, async browser => {
    const rejected = await SpecialPowers.spawn(browser, [AT_URI], async uri => {
      try {
        await content.fetch(uri);
        return false;
      } catch (e) {
        return true;
      }
    });
    Assert.ok(rejected, "fetch() must not accept at:// URIs");
  });
});

add_task(async function at_uri_is_not_an_external_protocol() {
  const external = Cc[
    "@mozilla.org/uriloader/external-protocol-service;1"
  ].getService(Ci.nsIExternalProtocolService);
  Assert.ok(
    !external.externalProtocolHandlerExists("at"),
    "at: must be handled internally rather than by the OS"
  );
  Assert.ok(PlacesUtils.history.canAddURI(Services.io.newURI(AT_URI)));
});

add_task(async function navigation_is_opaque_and_has_no_verified_ui() {
  await BrowserTestUtils.withNewTab(PAGE_URL, async browser => {
    const loaded = BrowserTestUtils.browserLoaded(browser, false, AT_URI);
    await SpecialPowers.spawn(browser, [], () => {
      content.document.getElementById("at-link").click();
    });
    await loaded;
    Assert.equal(browser.currentURI.spec, AT_URI);

    const state = await SpecialPowers.spawn(browser, [], () => ({
      origin: content.origin,
      secure: content.isSecureContext,
      title: content.document.title,
      body: content.document.body.textContent,
    }));
    Assert.equal(state.origin, "null");
    Assert.equal(state.secure, false);
    Assert.ok(/Raindrop/.test(state.title));
    Assert.ok(/failed|unavailable|error/i.test(state.body));

    const identityMode = document.getElementById("identity-box").className;
    Assert.ok(
      !identityMode.includes("verifiedDomain"),
      `unexpected identity mode: ${identityMode}`
    );
  });
});

add_task(async function link_context_actions_are_available() {
  await BrowserTestUtils.withNewTab(PAGE_URL, async browser => {
    const point = await SpecialPowers.spawn(browser, [], () => {
      const rect = content.document
        .getElementById("at-link")
        .getBoundingClientRect();
      return { x: rect.left + rect.width / 2, y: rect.top + rect.height / 2 };
    });
    const menu = document.getElementById("contentAreaContextMenu");
    const shown = BrowserTestUtils.waitForEvent(menu, "popupshown");
    await BrowserTestUtils.synthesizeMouseAtPoint(
      point.x,
      point.y,
      { type: "mousedown", button: 2 },
      browser
    );
    await BrowserTestUtils.synthesizeMouseAtPoint(
      point.x,
      point.y,
      { type: "contextmenu", button: 2 },
      browser
    );
    await shown;

    const copyLink = document.getElementById("context-copylink");
    const saveLink = document.getElementById("context-savelink");
    Assert.ok(BrowserTestUtils.isVisible(copyLink));
    Assert.ok(BrowserTestUtils.isVisible(saveLink));

    copyLink.doCommand();
    Assert.equal(readClipboardText(), AT_URI);

    const hidden = BrowserTestUtils.waitForEvent(menu, "popuphidden");
    menu.hidePopup();
    await hidden;
  });
});
