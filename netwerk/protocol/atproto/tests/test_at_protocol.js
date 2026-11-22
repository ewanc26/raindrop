/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const { NetUtil } = ChromeUtils.importESModule(
  "resource://gre/modules/NetUtil.sys.mjs"
);

const DID_URI =
  "at://did:plc:vwzwgnygau7ed7b7wt5ux7y2/app.bsky.feed.post/3k5nobkf2w72g";
const HANDLE_URI = "at://jay.bsky.social/app.bsky.feed.post/3k5nobkf2w72g";
const REPO_URI = "at://ewancroft.uk";
const COLLECTION_URI =
  "at://did:plc:vwzwgnygau7ed7b7wt5ux7y2/app.bsky.feed.post";
const CURSOR_URI =
  "at://did:plc:vwzwgnygau7ed7b7wt5ux7y2/app.bsky.feed.post/cursor/3n2tjsaiwrs24";
const INVALID_URI = "at://foo.com/";
const AT_CSP =
  "default-src 'none'; script-src 'none'; style-src 'unsafe-inline'; " +
  "img-src https:; object-src 'none'; base-uri 'none'; form-action 'none'; " +
  "frame-ancestors 'none'";

add_task(async function protocol_handler_is_registered() {
  const handler = Services.io.getProtocolHandler("at");
  Assert.notEqual(handler, Services.io.getProtocolHandler("default"));
  Assert.equal(handler.scheme, "at");
});

add_task(async function did_authority_round_trips() {
  const uri = Services.io.newURI(DID_URI);
  Assert.equal(uri.spec, DID_URI);
  Assert.equal(uri.scheme, "at");
  Assert.throws(() => uri.host, /NS_ERROR_FAILURE/);
  Assert.throws(() => uri.port, /NS_ERROR_FAILURE/);
});

add_task(async function handle_authority_round_trips() {
  const uri = Services.io.newURI(HANDLE_URI);
  Assert.equal(uri.spec, HANDLE_URI);
  Assert.equal(uri.scheme, "at");
  Assert.throws(() => uri.host, /NS_ERROR_FAILURE/);
});

add_task(async function opaque_authority_is_not_reparsed() {
  const uri = Services.io.newURI(DID_URI);
  const path = uri.pathQueryRef;
  Assert.ok(path.startsWith("//did:plc:vwzwgnygau7ed7b7wt5ux7y2/"));
});

add_task(async function record_uri_creates_an_html_channel() {
  // Creating the channel must succeed; the network fetches only start on
  // AsyncOpen. This keeps the test offline while checking NewChannel wiring.
  const chan = NetUtil.newChannel({
    uri: DID_URI,
    loadUsingSystemPrincipal: true,
  });
  Assert.equal(chan.URI.spec, DID_URI);
  Assert.equal(chan.contentType, "text/html");
});

add_task(async function generated_documents_set_security_headers() {
  const chan = NetUtil.newChannel({
    uri: DID_URI,
    loadUsingSystemPrincipal: true,
  });
  const http = chan.QueryInterface(Ci.nsIHttpChannel);
  Assert.equal(http.getResponseHeader("Content-Security-Policy"), AT_CSP);
  Assert.equal(http.getResponseHeader("X-Content-Type-Options"), "nosniff");
  Assert.equal(http.getResponseHeader("Referrer-Policy"), "no-referrer");
  Assert.equal(http.getResponseHeader("X-Frame-Options"), "DENY");
});

add_task(async function malformed_uri_rejects_channel_creation() {
  Assert.throws(
    () =>
      NetUtil.newChannel({
        uri: INVALID_URI,
        loadUsingSystemPrincipal: true,
      }),
    error => error.result === Cr.NS_ERROR_MALFORMED_URI
  );
});

add_task(async function handle_uri_creates_an_html_channel() {
  const chan = NetUtil.newChannel({
    uri: HANDLE_URI,
    loadUsingSystemPrincipal: true,
  });
  Assert.equal(chan.URI.spec, HANDLE_URI);
  Assert.equal(chan.contentType, "text/html");
});

add_task(async function at_uri_is_classified_as_a_url_not_a_search() {
  // ADR 0001: typing an at:// URI in the address bar must be classified as a
  // URL, not a search query, because the scheme has a registered handler.
  // Allowing keyword lookup is what makes this a meaningful classification
  // test: an unhandled scheme would be turned into keywordAsSent instead.
  for (const spec of [DID_URI, HANDLE_URI, REPO_URI, COLLECTION_URI]) {
    const info = Services.uriFixup.getFixupURIInfo(
      spec,
      Services.uriFixup.FIXUP_FLAG_ALLOW_KEYWORD_LOOKUP
    );
    Assert.equal(info.preferredURI.spec, spec);
    Assert.equal(info.fixedURI.spec, spec);
    Assert.equal(info.keywordAsSent, "");
  }
});

add_task(async function at_uri_is_loadable_by_web_content() {
  // at:// is a navigable content scheme like http/https/data, so web content
  // must be able to follow at:// links. URI_DANGEROUS_TO_LOAD made
  // CheckLoadURIWithPrincipal deny every such navigation with
  // NS_ERROR_DOM_BAD_URI; URI_LOADABLE_BY_ANYONE is the correct class.
  const PH = Ci.nsIProtocolHandler;
  const flags = Services.io.getProtocolFlags("at");
  Assert.ok(flags & PH.URI_LOADABLE_BY_ANYONE, "at: is loadable by anyone");
  Assert.ok(
    !(flags & PH.URI_DANGEROUS_TO_LOAD),
    "at: is not dangerous to load"
  );
  Assert.ok(
    !(flags & PH.URI_NON_PERSISTABLE),
    "at: is persistable (bookmarks/history)"
  );

  const ssm = Services.scriptSecurityManager;
  const web = ssm.createContentPrincipalFromOrigin("https://example.com");
  for (const spec of [DID_URI, HANDLE_URI, REPO_URI, COLLECTION_URI]) {
    const uri = Services.io.newURI(spec);
    ssm.checkLoadURIWithPrincipal(web, uri, 0);
    Assert.ok(true, `web content may load ${spec}`);
  }
});

add_task(async function authority_only_uri_creates_an_html_channel() {
  for (const spec of ["at://did:plc:vwzwgnygau7ed7b7wt5ux7y2", REPO_URI]) {
    const chan = NetUtil.newChannel({
      uri: spec,
      loadUsingSystemPrincipal: true,
    });
    Assert.equal(chan.URI.spec, spec);
    Assert.equal(chan.contentType, "text/html");
  }
});

add_task(async function collection_uri_creates_an_html_channel() {
  const chan = NetUtil.newChannel({
    uri: COLLECTION_URI,
    loadUsingSystemPrincipal: true,
  });
  Assert.equal(chan.URI.spec, COLLECTION_URI);
  Assert.equal(chan.contentType, "text/html");
});

add_task(async function cursor_uri_creates_an_html_channel() {
  const chan = NetUtil.newChannel({
    uri: CURSOR_URI,
    loadUsingSystemPrincipal: true,
  });
  Assert.equal(chan.URI.spec, CURSOR_URI);
  Assert.equal(chan.contentType, "text/html");

  const encoded =
    "at://did:plc:vwzwgnygau7ed7b7wt5ux7y2/app.bsky.feed.post/cursor/abc%3D";
  Assert.equal(Services.io.newURI(encoded).spec, encoded);
});

add_task(async function enabled_pref_controls_channel_creation() {
  const pref = "network.atprotocol.enabled";
  const original = Services.prefs.getBoolPref(pref);
  const hadUserValue = Services.prefs.prefHasUserValue(pref);
  try {
    Services.prefs.setBoolPref(pref, false);
    Assert.throws(
      () =>
        NetUtil.newChannel({
          uri: DID_URI,
          loadUsingSystemPrincipal: true,
        }),
      /NS_ERROR_NOT_AVAILABLE|0x80040111/
    );

    Services.prefs.setBoolPref(pref, true);
    const chan = NetUtil.newChannel({
      uri: DID_URI,
      loadUsingSystemPrincipal: true,
    });
    Assert.equal(chan.URI.spec, DID_URI);
  } finally {
    if (hadUserValue) {
      Services.prefs.setBoolPref(pref, original);
    } else {
      Services.prefs.clearUserPref(pref);
    }
  }
});

add_task(async function inline_images_default_to_off() {
  const pref = "network.atprotocol.record.showImages";
  const original = Services.prefs.getBoolPref(pref);
  const hadUserValue = Services.prefs.prefHasUserValue(pref);
  try {
    Services.prefs.clearUserPref(pref);
    Assert.equal(
      Services.prefs.getBoolPref(pref),
      false,
      "inline image loading is off by default"
    );
  } finally {
    if (hadUserValue) {
      Services.prefs.setBoolPref(pref, original);
    } else {
      Services.prefs.clearUserPref(pref);
    }
  }
});

add_task(async function view_style_defaults_are_safe() {
  const defaults = {
    "network.atprotocol.view.theme": "system",
    "network.atprotocol.view.textSize": "medium",
    "network.atprotocol.view.width": "standard",
    "network.atprotocol.view.accent": "blue",
    "network.atprotocol.view.density": "comfortable",
    "network.atprotocol.collection.pageSize": "50",
  };
  for (let [pref, expected] of Object.entries(defaults)) {
    let original = Services.prefs.getCharPref(pref);
    let hadUserValue = Services.prefs.prefHasUserValue(pref);
    try {
      Services.prefs.clearUserPref(pref);
      Assert.equal(
        Services.prefs.getCharPref(pref),
        expected,
        `${pref} has a safe default`
      );
    } finally {
      if (hadUserValue) {
        Services.prefs.setCharPref(pref, original);
      } else {
        Services.prefs.clearUserPref(pref);
      }
    }
  }
});
