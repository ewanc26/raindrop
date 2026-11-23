/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "AtDocumentPolicy.h"
#include "CollectionView.h"
#include "gtest/gtest.h"

using namespace mozilla::net::atproto;

namespace {

constexpr const char* kCollectionJson =
    R"({"records":[)"
    R"({"uri":"at://did:plc:abc/app.bsky.feed.post/3k5nobkf2w72g",)"
    R"("cid":"bafyreiexample","value":{"$type":"app.bsky.feed.post",)"
    R"("text":"hello","createdAt":"2026-01-01T00:00:00Z"}},)"
    R"({"uri":"at://did:plc:abc/app.bsky.feed.post/second",)"
    R"("cid":"bafyreisecond","value":{"$type":"app.bsky.actor.profile",)"
    R"("displayName":"Bob"}}],)"
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
  EXPECT_EQ(data.mRecords[0].mSummaryType, "app.bsky.feed.post");
  EXPECT_EQ(data.mRecords[0].mSummaryText, "hello");
  EXPECT_EQ(data.mRecords[0].mSummaryWhen, "2026-01-01T00:00:00Z");
  EXPECT_EQ(data.mRecords[1].mRecordKey, "second");
  EXPECT_EQ(data.mRecords[1].mSummaryType, "app.bsky.actor.profile");
  EXPECT_EQ(data.mRecords[1].mSummaryText, "Bob");
  EXPECT_TRUE(data.mRecords[1].mSummaryWhen.empty());
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
  EXPECT_NE(html.find(kAtDocumentCsp), std::string::npos);
  EXPECT_NE(html.find("name=\"referrer\""), std::string::npos);
  EXPECT_NE(html.find("rel=\"icon\""), std::string::npos);
  EXPECT_NE(html.find("data:image/svg+xml;base64,"), std::string::npos);
  EXPECT_NE(html.find("rd-header"), std::string::npos);
  EXPECT_NE(html.find("direct at:// fetch"), std::string::npos);
  EXPECT_NE(html.find("Collection"), std::string::npos);
  EXPECT_NE(html.find("app.bsky.feed.post"), std::string::npos);
  EXPECT_NE(html.find("alice.example"), std::string::npos);
  EXPECT_NE(html.find("https://pds.example.com"), std::string::npos);
  EXPECT_NE(html.find("href=\"at://did:plc:abc/app.bsky.feed.post/"
                      "3k5nobkf2w72g\""),
            std::string::npos);
  EXPECT_NE(html.find("bafyreiexample"), std::string::npos);
  EXPECT_NE(html.find("app.bsky.feed.post — hello "
                      "(2026-01-01T00:00:00Z)"),
            std::string::npos);
  EXPECT_NE(html.find("app.bsky.actor.profile — Bob"), std::string::npos);
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

TEST(CollectionView, EscapesAndCapsSummaries)
{
  const std::string json =
      R"({"records":[{"uri":"at://did:plc:abc/app.bsky.feed.post/k",)"
      R"("value":{"$type":"app.bsky.feed.post",)"
      R"("text":"<script>alert('x')</script> & \"quoted\"",)"
      R"("createdAt":"2026-01-01T00:00:00Z"}}]})";
  CollectionViewData data;
  ASSERT_EQ(ParseCollectionResponse(json, data), CollectionViewError::None);
  std::string html = RenderCollectionHtml(data, kProvenance);
  EXPECT_EQ(html.find("<script>"), std::string::npos);
  EXPECT_NE(html.find("&lt;script&gt;alert(&#39;x&#39;)&lt;/script&gt;"),
            std::string::npos);
  EXPECT_NE(html.find("&amp; &quot;quoted&quot;"), std::string::npos);

  const std::string longJson =
      R"({"records":[{"uri":"at://did:plc:abc/app.bsky.feed.post/long",)"
      R"("value":{"text":")" +
      std::string(200, 'x') + R"("}}]})";
  ASSERT_EQ(ParseCollectionResponse(longJson, data), CollectionViewError::None);
  ASSERT_EQ(data.mRecords.size(), size_t(1));
  EXPECT_EQ(data.mRecords[0].mSummaryText.size(), size_t(163));
  html = RenderCollectionHtml(data, kProvenance);
  EXPECT_NE(html.find(std::string(160, 'x') + "\xe2\x80\xa6"),
            std::string::npos);
  EXPECT_EQ(html.find(std::string(200, 'x')), std::string::npos);

  std::string emoji;
  for (int i = 0; i < 80; ++i) {
    emoji += "\xf0\x9f\x98\x80";
  }
  const std::string emojiJson =
      R"({"records":[{"uri":"at://did:plc:abc/app.bsky.feed.post/emoji",)"
      R"("value":{"text":")" +
      emoji + R"("}}]})";
  ASSERT_EQ(ParseCollectionResponse(emojiJson, data),
            CollectionViewError::None);
  EXPECT_EQ(data.mRecords[0].mSummaryText.size(), size_t(163));
  EXPECT_EQ(data.mRecords[0].mSummaryText.substr(160), "\xe2\x80\xa6");
  EXPECT_EQ(data.mRecords[0].mSummaryText.find("\xef\xbf\xbd"),
            std::string::npos);
}

TEST(CollectionView, KeepsProvenanceLinksWhenPdsReturnsOtherDid)
{
  const std::string json =
      R"({"records":[{"uri":"at://did:plc:evil/app.bsky.feed.post/k",)"
      R"("cid":"cexample","value":{"$type":"app.bsky.feed.post",)"
      R"("text":"summary"}}]})";
  CollectionViewData data;
  ASSERT_EQ(ParseCollectionResponse(json, data), CollectionViewError::None);
  std::string html = RenderCollectionHtml(data, kProvenance);
  EXPECT_NE(html.find("href=\"at://did:plc:abc/app.bsky.feed.post/k\""),
            std::string::npos);
  EXPECT_EQ(html.find("did:plc:evil"), std::string::npos);
  EXPECT_NE(html.find("app.bsky.feed.post — summary"), std::string::npos);
}

TEST(CollectionView, IgnoresUnhelpfulRecordValues)
{
  const std::string json =
      R"({"records":[)"
      R"({"uri":"at://did:plc:abc/app.bsky.feed.post/n1","value":42},)"
      R"({"uri":"at://did:plc:abc/app.bsky.feed.post/n2","value":[]},)"
      R"({"uri":"at://did:plc:abc/app.bsky.feed.post/n3","value":{}},)"
      R"({"uri":"at://did:plc:abc/app.bsky.feed.post/n4",)"
      R"("value":{"$type":"not-a-nsid"}}]})";
  CollectionViewData data;
  ASSERT_EQ(ParseCollectionResponse(json, data), CollectionViewError::None);
  ASSERT_EQ(data.mRecords.size(), size_t(4));
  for (const CollectionViewRecord& record : data.mRecords) {
    EXPECT_TRUE(record.mSummaryType.empty());
    EXPECT_TRUE(record.mSummaryText.empty());
    EXPECT_TRUE(record.mSummaryWhen.empty());
  }
  std::string html = RenderCollectionHtml(data, kProvenance);
  EXPECT_EQ(html.find("<div class=\"hint\">"), std::string::npos);
}
