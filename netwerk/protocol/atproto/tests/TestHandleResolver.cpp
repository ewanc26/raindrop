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

void ExpectTxtHost(std::string_view aHandle, std::string_view aExpected) {
  std::string host;
  EXPECT_EQ(BuildHandleTxtHost(aHandle, host), HandleError::None) << aHandle;
  EXPECT_EQ(host, aExpected) << aHandle;
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
  ExpectBinding(json, "other.example", HandleBindingError::HandleNotClaimed);
  ExpectBinding(json, "mallory.example",
                HandleBindingError::HandleNotClaimed);
}

TEST(HandleResolver, OnlyFirstValidHandleIsClaimed) {
  const std::string json = R"({
    "alsoKnownAs": ["at://first.example", "at://second.example"]
  })";
  ExpectBinding(json, "first.example", HandleBindingError::None);
  ExpectBinding(json, "second.example", HandleBindingError::HandleNotClaimed);
}

TEST(HandleResolver, SkipsInvalidEntriesBeforeClaimedHandle) {
  const std::string json = R"({
    "alsoKnownAs": [
      "https://example.com/alice",
      "at://singlelabel",
      "at://user:pass@example.com",
      "at://alice.example",
      "at://later.example"
    ]
  })";
  ExpectBinding(json, "alice.example", HandleBindingError::None);
  ExpectBinding(json, "later.example",
                HandleBindingError::HandleNotClaimed);
}

TEST(HandleResolver, RejectsHandleWithUriPartsInClaim) {
  const std::string json = R"({"alsoKnownAs":["at://alice.example/path"]})";
  ExpectBinding(json, "alice.example", HandleBindingError::HandleNotClaimed);
}

TEST(HandleResolver, BindingComparisonIsCaseInsensitive) {
  const std::string json = R"({"alsoKnownAs":["at://Alice.Example.COM"]})";
  ExpectBinding(json, "alice.example.com", HandleBindingError::None);
  ExpectBinding(json, "ALICE.EXAMPLE.COM", HandleBindingError::None);
  ExpectBinding(R"({"alsoKnownAs":["AT://alice.example.com"]})",
                "alice.example.com", HandleBindingError::None);
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

TEST(HandleResolver, BuildsDnsTxtHost) {
  ExpectTxtHost("alice.example", "_atproto.alice.example");
  ExpectTxtHost("jay.bsky.social", "_atproto.jay.bsky.social");
}

TEST(HandleResolver, RejectsInvalidHandleForTxtHost) {
  std::string host;
  EXPECT_EQ(BuildHandleTxtHost("singlelabel", host),
            HandleError::InvalidHandle);
  EXPECT_EQ(BuildHandleTxtHost("bad_label.example", host),
            HandleError::InvalidHandle);
  EXPECT_TRUE(host.empty());
}

TEST(HandleResolver, ParsesDnsTxtDid) {
  std::string did;
  const std::string expected = "did:plc:vwzwgnygau7ed7b7wt5ux7y2";
  EXPECT_EQ(ParseHandleTxtRecords({"did=" + expected}, did), HandleError::None);
  EXPECT_EQ(did, expected);
  EXPECT_EQ(ParseHandleTxtRecords({" did=" + expected + " "}, did),
            HandleError::None);
  EXPECT_EQ(did, expected);
  // Unrelated records are ignored, as atproto permits other TXT entries.
  EXPECT_EQ(ParseHandleTxtRecords({"v=spf1 -all", "did=" + expected}, did),
            HandleError::None);
  EXPECT_EQ(did, expected);
}

TEST(HandleResolver, RejectsBadDnsTxtRecords) {
  std::string did;
  EXPECT_EQ(ParseHandleTxtRecords({}, did), HandleError::TxtNoDid);
  EXPECT_EQ(ParseHandleTxtRecords({"v=spf1 -all"}, did), HandleError::TxtNoDid);
  EXPECT_EQ(ParseHandleTxtRecords({"did=not a did"}, did),
            HandleError::InvalidResponse);
  EXPECT_EQ(ParseHandleTxtRecords(
                {"did=did:plc:aaa", "did=did:plc:bbb"}, did),
            HandleError::TxtMultipleDid);
  EXPECT_TRUE(did.empty());
  std::string tooLong(kMaxHandleResponseLength + 1, 'a');
  EXPECT_EQ(ParseHandleTxtRecords({tooLong}, did), HandleError::TooLong);
}
