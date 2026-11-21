/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "CollectionView.h"
#include "gtest/gtest.h"

using namespace mozilla::net::atproto;

namespace {

constexpr const char* kCollectionJson =
    R"({"records":[)"
    R"({"uri":"at://did:plc:abc/app.bsky.feed.post/3k5nobkf2w72g",)"
    R"("cid":"bafyreiexample","value":{"text":"hello"}},)"
    R"({"uri":"at://did:plc:abc/app.bsky.feed.post/second","cid":"bafyreisecond"}],)"
    R"("cursor":"next-page"})";

const CollectionViewProvenance kProvenance{"did:plc:abc", "alice.example",
                                           "https://pds.example.com",
                                           "app.bsky.feed.post", ""};

}  // namespace

TEST(CollectionView, ParsesRecordsAndCursor)
{
  CollectionViewData data;
  EXPECT_EQ(ParseCollectionResponse(kCollectionJson, data),
            CollectionViewError::None);
  ASSERT_EQ(data.mRecords.size(), size_t(2));
  EXPECT_EQ(data.mRecords[0].mRecordKey, "3k5nobkf2w72g");
  EXPECT_EQ(data.mRecords[0].mCid, "bafyreiexample");
  EXPECT_EQ(data.mRecords[1].mRecordKey, "second");
  EXPECT_EQ(data.mCursor, "next-page");
}

TEST(CollectionView, AcceptsEmptyListing)
{
  CollectionViewData data;
  EXPECT_EQ(ParseCollectionResponse(R"({"records":[]})", data),
            CollectionViewError::None);
  EXPECT_TRUE(data.mRecords.empty());
  EXPECT_TRUE(data.mCursor.empty());
}

TEST(CollectionView, RejectsInvalidResponses)
{
  CollectionViewData data;
  EXPECT_EQ(ParseCollectionResponse("not json", data),
            CollectionViewError::InvalidJson);
  EXPECT_EQ(ParseCollectionResponse("[]", data),
            CollectionViewError::InvalidJson);
  EXPECT_EQ(ParseCollectionResponse(R"({"cursor":"next"})", data),
            CollectionViewError::MissingRecords);
  EXPECT_EQ(ParseCollectionResponse(R"({"records":[42]})", data),
            CollectionViewError::InvalidRecord);
  EXPECT_EQ(ParseCollectionResponse(R"({"records":[{"cid":"abc"}]})", data),
            CollectionViewError::InvalidRecord);
  EXPECT_EQ(ParseCollectionResponse(
                R"({"records":[{"uri":"https://evil.example/x"}]})", data),
            CollectionViewError::InvalidRecord);
  EXPECT_EQ(ParseCollectionResponse(
                std::string(kMaxCollectionResponseLength + 1, 'x'), data),
            CollectionViewError::TooLong);
}

TEST(CollectionView, RendersEscapedLinksBuiltFromProvenance)
{
  CollectionViewData data;
  ASSERT_EQ(ParseCollectionResponse(kCollectionJson, data),
            CollectionViewError::None);

  std::string html = RenderCollectionHtml(data, kProvenance);
  EXPECT_NE(html.find("Collection"), std::string::npos);
  EXPECT_NE(html.find("app.bsky.feed.post"), std::string::npos);
  EXPECT_NE(html.find("alice.example"), std::string::npos);
  EXPECT_NE(html.find("https://pds.example.com"), std::string::npos);
  EXPECT_NE(html.find("href=\"at://did:plc:abc/app.bsky.feed.post/"
                      "3k5nobkf2w72g\""),
            std::string::npos);
  EXPECT_NE(html.find("bafyreiexample"), std::string::npos);
  EXPECT_NE(html.find("No gateway was contacted"), std::string::npos);
  EXPECT_NE(html.find("href=\"at://did:plc:abc/app.bsky.feed.post/cursor/"
                      "next-page\""),
            std::string::npos);
  EXPECT_NE(html.find("Next page"), std::string::npos);
  EXPECT_EQ(html.find("First page"), std::string::npos);
}

TEST(CollectionView, EscapesUntrustedProvenanceAndIdentifiers)
{
  CollectionViewData data;
  CollectionViewProvenance provenance = kProvenance;
  data.mRecords.push_back({"rkey\"onmouseover=\"evil", "<script>cid</script>"});
  provenance.mDid = "<did>";
  provenance.mCollection = "a&b";

  std::string html = RenderCollectionHtml(data, provenance);
  EXPECT_EQ(html.find("<script>"), std::string::npos);
  EXPECT_EQ(html.find("onmouseover=\"evil\""), std::string::npos);
  EXPECT_NE(html.find("&lt;did&gt;"), std::string::npos);
  EXPECT_NE(html.find("a&amp;b"), std::string::npos);
  EXPECT_NE(html.find("rkey&quot;onmouseover=&quot;evil"), std::string::npos);
  EXPECT_NE(html.find("&lt;script&gt;cid&lt;/script&gt;"), std::string::npos);
}

TEST(CollectionView, ShowsEmptyListWithoutNextLink)
{
  std::string html = RenderCollectionHtml(CollectionViewData{}, kProvenance);
  EXPECT_NE(html.find("(no records)"), std::string::npos);
  EXPECT_EQ(html.find("Next page"), std::string::npos);
  EXPECT_EQ(html.find("First page"), std::string::npos);
}

TEST(CollectionView, RendersFirstPageLinkWhenPaged)
{
  CollectionViewData data;
  ASSERT_EQ(ParseCollectionResponse(kCollectionJson, data),
            CollectionViewError::None);
  CollectionViewProvenance provenance = kProvenance;
  provenance.mCursor = "3n2tjsaiwrs24";

  std::string html = RenderCollectionHtml(data, provenance);
  EXPECT_NE(html.find("href=\"at://did:plc:abc/app.bsky.feed.post\""),
            std::string::npos);
  EXPECT_NE(html.find("First page"), std::string::npos);
}

TEST(CollectionView, EncodesCursorInNextLink)
{
  CollectionViewData data;
  ASSERT_EQ(ParseCollectionResponse(R"({"records":[],"cursor":"a=b&c"})", data),
            CollectionViewError::None);

  std::string html = RenderCollectionHtml(data, kProvenance);
  EXPECT_NE(html.find("/cursor/a%3Db%26c\""), std::string::npos);
  EXPECT_EQ(html.find("cursor/a=b&c"), std::string::npos);
}
