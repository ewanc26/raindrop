/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DidResolver.h"

#include "gtest/gtest.h"

using namespace mozilla::net::atproto;

namespace {

void ExpectDidUrl(std::string_view aDid, std::string_view aExpected) {
  std::string url;
  EXPECT_EQ(BuildDidDocumentUrl(aDid, url), DidUrlError::None) << aDid;
  EXPECT_EQ(url, aExpected) << aDid;
}

void ExpectPds(std::string_view aJson, std::string_view aDid,
               std::string_view aExpected) {
  PdsService service;
  EXPECT_EQ(ExtractPdsEndpoint(aJson, aDid, service), PdsError::None);
  EXPECT_EQ(service.mEndpoint, aExpected);
}

}  // namespace

TEST(DidResolver, ParsePlc) {
  Did did;
  EXPECT_EQ(ParseDid("did:plc:vwzwgnygau7ed7b7wt5ux7y2", did), DidError::None);
  EXPECT_EQ(did.mMethod, DidMethod::Plc);
  EXPECT_EQ(did.mId, "vwzwgnygau7ed7b7wt5ux7y2");
  EXPECT_EQ(did.mFull, "did:plc:vwzwgnygau7ed7b7wt5ux7y2");
}

TEST(DidResolver, ParseWeb) {
  Did did;
  EXPECT_EQ(ParseDid("did:web:user.example.com", did), DidError::None);
  EXPECT_EQ(did.mMethod, DidMethod::Web);
  EXPECT_EQ(did.mId, "user.example.com");
  EXPECT_EQ(ParseDid("did:web:example.com%3A3000:u:alice", did),
            DidError::None);
  EXPECT_EQ(did.mMethod, DidMethod::Web);
}

TEST(DidResolver, UnsupportedAndInvalid) {
  Did did;
  EXPECT_EQ(ParseDid("did:key:z6Mki", did), DidError::UnsupportedMethod);
  EXPECT_EQ(ParseDid("did:plc:", did), DidError::InvalidSyntax);
  EXPECT_EQ(ParseDid("did:web:singlelabel", did),
            DidError::InvalidIdentifier);
  EXPECT_EQ(ParseDid("did:web:example.com:..", did),
            DidError::InvalidIdentifier);
  EXPECT_EQ(ParseDid("did:web:example.com%2Fetc", did),
            DidError::InvalidIdentifier);
  EXPECT_EQ(ParseDid("did:web:example.com%3A99999", did),
            DidError::InvalidIdentifier);
  EXPECT_EQ(ParseDid("did:web:user:pass@example.com", did),
            DidError::InvalidSyntax);
}

TEST(DidResolver, DocumentUrls) {
  ExpectDidUrl("did:plc:vwzwgnygau7ed7b7wt5ux7y2",
               "https://plc.directory/did:plc:vwzwgnygau7ed7b7wt5ux7y2");
  ExpectDidUrl("did:web:example.com", "https://example.com/.well-known/did.json");
  ExpectDidUrl("did:web:user.example.com",
               "https://user.example.com/.well-known/did.json");
  ExpectDidUrl("did:web:example.com%3A3000",
               "https://example.com:3000/.well-known/did.json");
  ExpectDidUrl("did:web:example.com:user:alice",
               "https://example.com/user/alice/did.json");
}

TEST(DidResolver, ExtractPds) {
  const std::string did = "did:plc:vwzwgnygau7ed7b7wt5ux7y2";
  const std::string json = R"({
    "id": "did:plc:vwzwgnygau7ed7b7wt5ux7y2",
    "service": [
      {"id": "#other", "type": "Other", "serviceEndpoint": "https://elsewhere"},
      {"id": "#atproto_pds", "type": "AtprotoPersonalDataServer",
       "serviceEndpoint": "https://pds.example.com"}
    ]
  })";
  ExpectPds(json, did, "https://pds.example.com");

  const std::string fullIdJson = R"({
    "id": "did:web:example.com",
    "service": [{"id": "did:web:example.com#atproto_pds",
                 "serviceEndpoint": "https://pds.example.com"}]
  })";
  ExpectPds(fullIdJson, "did:web:example.com", "https://pds.example.com");
}

TEST(DidResolver, RejectsBadDocuments) {
  const std::string did = "did:plc:abc";
  PdsService service;
  EXPECT_EQ(ExtractPdsEndpoint("not json", did, service),
            PdsError::InvalidJson);
  EXPECT_EQ(ExtractPdsEndpoint("[]", did, service), PdsError::InvalidJson);
  EXPECT_EQ(ExtractPdsEndpoint(R"({"service":[]})", did, service),
            PdsError::MissingId);
  EXPECT_EQ(ExtractPdsEndpoint(R"({"id":"did:plc:other","service":[]})", did,
                               service),
            PdsError::IdMismatch);
  EXPECT_EQ(ExtractPdsEndpoint(R"({"id":"did:plc:abc"})", did, service),
            PdsError::MissingService);
  EXPECT_EQ(
      ExtractPdsEndpoint(
          R"({"id":"did:plc:abc","service":[{"id":"#atproto_pds","serviceEndpoint":"http://pds.example.com"}]})",
          did, service),
      PdsError::InvalidEndpoint);
  EXPECT_EQ(
      ExtractPdsEndpoint(
          R"({"id":"did:plc:abc","service":[{"id":"#atproto_pds","serviceEndpoint":"https://user@pds.example.com"}]})",
          did, service),
      PdsError::InvalidEndpoint);
  EXPECT_EQ(
      ExtractPdsEndpoint(
          R"({"id":"did:plc:abc","service":[{"id":"#atproto_pds","serviceEndpoint":"https://pds.example.com/?x=1"}]})",
          did, service),
      PdsError::InvalidEndpoint);
  EXPECT_EQ(
      ExtractPdsEndpoint(
          R"({"id":"did:plc:abc","service":[{"id":"#atproto_pds","serviceEndpoint":"https://pds.example.com#frag"}]})",
          did, service),
      PdsError::InvalidEndpoint);
  std::string tooLarge(kMaxDidDocumentLength + 1, ' ');
  EXPECT_EQ(ExtractPdsEndpoint(tooLarge, did, service), PdsError::TooLarge);
}

TEST(DidResolver, OnionEndpointIsDistinctFailure) {
  const std::string did = "did:plc:abc";
  PdsService service;
  const std::string json =
      R"({"id":"did:plc:abc","service":[{"id":"#atproto_pds",
           "serviceEndpoint":"https://abcdef123456.onion"}]})";
  EXPECT_EQ(ExtractPdsEndpoint(json, did, service), PdsError::OnionEndpoint);
  EXPECT_EQ(service.mEndpoint, "https://abcdef123456.onion");
}

TEST(DidResolver, OnionSuffixIsCaseInsensitive) {
  const std::string did = "did:plc:abc";
  PdsService service;
  const std::string json =
      R"({"id":"did:plc:abc","service":[{"id":"#atproto_pds",
           "serviceEndpoint":"https://ABCDEF123456.ONION"}]})";
  EXPECT_EQ(ExtractPdsEndpoint(json, did, service), PdsError::OnionEndpoint);
}

TEST(DidResolver, ParseDidCanonicalizesPercentEscapeCase) {
  Did did;
  ASSERT_EQ(ParseDid("did:web:alice.example%3a8443", did), DidError::None);
  EXPECT_EQ(did.mFull, "did:web:alice.example%3A8443");
  EXPECT_EQ(did.mId, "alice.example%3A8443");
}

TEST(DidResolver, HandleResolutionMatchesCanonicalDocumentId) {
  // A handle's TXT/well-known response can spell the hex escape in lowercase,
  // while the DID document's id uses the canonical uppercase form.
  Did did;
  ASSERT_EQ(ParseDid("did:web:alice.example%3a8443", did), DidError::None);
  const std::string json =
      R"({"id":"did:web:alice.example%3A8443","service":[
           {"id":"#atproto_pds","serviceEndpoint":"https://pds.example.com"}]})";
  PdsService service;
  EXPECT_EQ(ExtractPdsEndpoint(json, did.mFull, service), PdsError::None);
  EXPECT_EQ(service.mEndpoint, "https://pds.example.com");
}
