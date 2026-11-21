/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "PdsClient.h"
#include "gtest/gtest.h"

using namespace mozilla::net::atproto;

namespace {

void ExpectUrl(std::string_view aEndpoint, std::string_view aRepo,
               std::string_view aCollection, std::string_view aRecordKey,
               std::string_view aExpected) {
  std::string url;
  EXPECT_EQ(BuildGetRecordUrl(aEndpoint, aRepo, aCollection, aRecordKey, url),
            PdsUrlError::None);
  EXPECT_EQ(url, aExpected);
}

}  // namespace

TEST(PdsClient, BuildGetRecordUrl)
{
  ExpectUrl("https://pds.example.com", "did:plc:vwzwgnygau7ed7b7wt5ux7y2",
            "app.bsky.feed.post", "3k5nobkf2w72g",
            "https://pds.example.com/xrpc/com.atproto.repo.getRecord"
            "?repo=did%3Aplc%3Avwzwgnygau7ed7b7wt5ux7y2"
            "&collection=app.bsky.feed.post&rkey=3k5nobkf2w72g");
  ExpectUrl("https://pds.example.com/", "did:web:user.example.com",
            "com.atproto.lexicon.schema", "com.atproto.lexicon.schema",
            "https://pds.example.com/xrpc/com.atproto.repo.getRecord"
            "?repo=did%3Aweb%3Auser.example.com"
            "&collection=com.atproto.lexicon.schema"
            "&rkey=com.atproto.lexicon.schema");
}

TEST(PdsClient, QueryValuesAreEncoded)
{
  EXPECT_EQ(EncodeQueryValue("did:plc:abc"), "did%3Aplc%3Aabc");
  EXPECT_EQ(EncodeQueryValue("a.b_c-d~e"), "a.b_c-d~e");
  EXPECT_EQ(EncodeQueryValue("a b/c?"), "a%20b%2Fc%3F");
}

TEST(PdsClient, RejectsInvalidInputs)
{
  std::string url;
  EXPECT_EQ(BuildGetRecordUrl("http://pds.example.com", "did:plc:abc",
                              "app.bsky.feed.post", "self", url),
            PdsUrlError::InvalidEndpoint);
  EXPECT_EQ(BuildGetRecordUrl("https://user@pds.example.com", "did:plc:abc",
                              "app.bsky.feed.post", "self", url),
            PdsUrlError::InvalidEndpoint);
  EXPECT_EQ(BuildGetRecordUrl("https://pds.example.com", "not-a-did",
                              "app.bsky.feed.post", "self", url),
            PdsUrlError::InvalidRepo);
  EXPECT_EQ(BuildGetRecordUrl("https://pds.example.com", "did:plc:abc",
                              "invalid", "self", url),
            PdsUrlError::InvalidCollection);
  EXPECT_EQ(BuildGetRecordUrl("https://pds.example.com", "did:plc:abc",
                              "app.bsky.feed.post", "../self", url),
            PdsUrlError::InvalidRecordKey);
}

TEST(PdsClient, BuildDescribeRepoUrl)
{
  std::string url;
  EXPECT_EQ(BuildDescribeRepoUrl("https://pds.example.com/",
                                 "did:plc:vwzwgnygau7ed7b7wt5ux7y2", url),
            PdsUrlError::None);
  EXPECT_EQ(url,
            "https://pds.example.com/xrpc/com.atproto.repo.describeRepo"
            "?repo=did%3Aplc%3Avwzwgnygau7ed7b7wt5ux7y2");
}

TEST(PdsClient, DescribeRepoRejectsInvalidInputs)
{
  std::string url;
  EXPECT_EQ(BuildDescribeRepoUrl("http://pds.example.com", "did:plc:abc", url),
            PdsUrlError::InvalidEndpoint);
  EXPECT_EQ(BuildDescribeRepoUrl("https://pds.example.com", "not-a-did", url),
            PdsUrlError::InvalidRepo);
}

TEST(PdsClient, BuildGetBlobUrl)
{
  std::string url;
  EXPECT_EQ(
      BuildGetBlobUrl(
          "https://pds.example.com/", "did:plc:vwzwgnygau7ed7b7wt5ux7y2",
          "bafyreidyc6ykjx5kurjt5kxdqcdxf5nl6qpsbpp2w4mzd3c6spkm4hdxq", url),
      PdsUrlError::None);
  EXPECT_EQ(url,
            "https://pds.example.com/xrpc/com.atproto.sync.getBlob"
            "?did=did%3Aplc%3Avwzwgnygau7ed7b7wt5ux7y2"
            "&cid=bafyreidyc6ykjx5kurjt5kxdqcdxf5nl6qpsbpp2w4mzd3c6spkm4hdxq");
}

TEST(PdsClient, BlobUrlRejectsInvalidInputs)
{
  std::string url;
  constexpr const char* kCid =
      "bafyreidyc6ykjx5kurjt5kxdqcdxf5nl6qpsbpp2w4mzd3c6spkm4hdxq";
  EXPECT_EQ(BuildGetBlobUrl("http://pds.example.com", "did:plc:abc", kCid, url),
            PdsUrlError::InvalidEndpoint);
  EXPECT_EQ(BuildGetBlobUrl("https://pds.example.com", "not-a-did", kCid, url),
            PdsUrlError::InvalidRepo);
  EXPECT_EQ(BuildGetBlobUrl("https://pds.example.com", "did:plc:abc",
                            "cid&repo=evil", url),
            PdsUrlError::InvalidCid);
  EXPECT_EQ(BuildGetBlobUrl("https://pds.example.com", "did:plc:abc", "", url),
            PdsUrlError::InvalidCid);
}

TEST(PdsClient, BlobUrlAcceptsBase58Cid)
{
  std::string url;
  EXPECT_EQ(
      BuildGetBlobUrl("https://pds.example.com", "did:web:user.example",
                      "QmYwAPJzv5CZsnAzt8auVZRnV7pFDxbmvG5jM5vGdcbqZa", url),
      PdsUrlError::None);
  EXPECT_EQ(url,
            "https://pds.example.com/xrpc/com.atproto.sync.getBlob"
            "?did=did%3Aweb%3Auser.example"
            "&cid=QmYwAPJzv5CZsnAzt8auVZRnV7pFDxbmvG5jM5vGdcbqZa");
}
