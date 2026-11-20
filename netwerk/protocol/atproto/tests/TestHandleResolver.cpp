/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "HandleResolver.h"

#include "DidResolver.h"
#include "gtest/gtest.h"

using namespace mozilla::net::atproto;

namespace {

void ExpectHandleUrl(std::string_view aHandle, std::string_view aExpected) {
  std::string url;
  EXPECT_EQ(BuildHandleDidUrl(aHandle, url), HandleError::None) << aHandle;
  EXPECT_EQ(url, aExpected) << aHandle;
}

void ExpectHandleDid(std::string_view aBody, std::string_view aExpected) {
  std::string did;
  EXPECT_EQ(ParseHandleDidResponse(aBody, did), HandleError::None) << aBody;
  EXPECT_EQ(did, aExpected) << aBody;
}

void ExpectBinding(std::string_view aJson, std::string_view aHandle,
                   HandleBindingError aExpected) {
  EXPECT_EQ(VerifyHandleBinding(aJson, aHandle), aExpected);
}

}  // namespace

TEST(HandleResolver, BuildHandleUrl) {
  ExpectHandleUrl("alice.example", "https://alice.example/.well-known/atproto-did");
  ExpectHandleUrl("jay.bsky.social",
                  "https://jay.bsky.social/.well-known/atproto-did");
}

TEST(HandleResolver, RejectsInvalidHandle) {
  std::string url;
  EXPECT_EQ(BuildHandleDidUrl("singlelabel", url),
            HandleError::InvalidHandle);
  EXPECT_EQ(BuildHandleDidUrl("bad_label.example", url),
            HandleError::InvalidHandle);
  EXPECT_EQ(BuildHandleDidUrl("user:pass@example.com", url),
            HandleError::InvalidHandle);
  EXPECT_TRUE(url.empty());
}

TEST(HandleResolver, ParsesDidResponse) {
  const std::string did = "did:plc:vwzwgnygau7ed7b7wt5ux7y2";
  ExpectHandleDid(did, did);
  ExpectHandleDid(did + "\n", did);
  ExpectHandleDid("  " + did + " \t\r\n", did);
  ExpectHandleDid("did:web:example.com", "did:web:example.com");
}

TEST(HandleResolver, RejectsBadDidResponse) {
  std::string did;
  EXPECT_EQ(ParseHandleDidResponse("", did), HandleError::InvalidResponse);
  EXPECT_EQ(ParseHandleDidResponse("   \n", did),
            HandleError::InvalidResponse);
  EXPECT_EQ(ParseHandleDidResponse("not a did", did),
            HandleError::InvalidResponse);
  EXPECT_EQ(ParseHandleDidResponse("did:key:z6Mki", did),
            HandleError::InvalidResponse);
  EXPECT_EQ(ParseHandleDidResponse("did:plc:abc extra", did),
            HandleError::InvalidResponse);
  std::string tooLong(kMaxHandleResponseLength + 1, 'a');
  EXPECT_EQ(ParseHandleDidResponse(tooLong, did), HandleError::TooLong);
}

TEST(HandleResolver, VerifiesReverseBinding) {
  const std::string json = R"({
    "id": "did:plc:abc",
    "alsoKnownAs": ["at://alice.example", "at://other.example"]
  })";
  ExpectBinding(json, "alice.example", HandleBindingError::None);
  ExpectBinding(json, "other.example", HandleBindingError::None);
  ExpectBinding(json, "mallory.example",
                HandleBindingError::HandleNotClaimed);
}

TEST(HandleResolver, RejectsBadBindingDocuments) {
  ExpectBinding("not json", "alice.example",
                HandleBindingError::InvalidJson);
  ExpectBinding(R"({"alsoKnownAs":"at://alice.example"})", "alice.example",
                HandleBindingError::MissingAlsoKnownAs);
  ExpectBinding(R"({"alsoKnownAs":[]})", "alice.example",
                HandleBindingError::HandleNotClaimed);
  ExpectBinding(R"({"alsoKnownAs":[123]})", "alice.example",
                HandleBindingError::HandleNotClaimed);
}
