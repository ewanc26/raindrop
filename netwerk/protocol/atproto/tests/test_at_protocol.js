/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const DID_URI =
  "at://did:plc:vwzwgnygau7ed7b7wt5ux7y2/app.bsky.feed.post/3k5nobkf2w72g";
const HANDLE_URI = "at://jay.bsky.social/app.bsky.feed.post/3k5nobkf2w72g";

add_task(async function protocol_handler_is_registered() {
  const handler = Services.io.getProtocolHandler("at");
  Assert.notEqual(handler, Services.io.getProtocolHandler("default"));
  Assert.equal(handler.scheme, "at");
});

add_task(async function did_authority_round_trips() {
  const uri = Services.io.newURI(DID_URI);
  Assert.equal(uri.spec, DID_URI);
  Assert.equal(uri.scheme, "at");
  Assert.throws(() => uri.host);
  Assert.throws(() => uri.port);
});

add_task(async function handle_authority_round_trips() {
  const uri = Services.io.newURI(HANDLE_URI);
  Assert.equal(uri.spec, HANDLE_URI);
  Assert.equal(uri.scheme, "at");
  Assert.throws(() => uri.host);
});

add_task(async function opaque_authority_is_not_reparsed() {
  const uri = Services.io.newURI(DID_URI);
  const path = uri.pathQueryRef;
  Assert.ok(path.startsWith("//did:plc:vwzwgnygau7ed7b7wt5ux7y2/"));
});
