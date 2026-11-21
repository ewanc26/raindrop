/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "LexiconView.h"
#include "gtest/gtest.h"

using namespace mozilla::net::atproto;

TEST(LexiconView, RendersPostTextAndDate)
{
  const std::string value =
      R"({"$type":"app.bsky.feed.post",)"
      R"("text":"hello world","createdAt":"2026-01-01T00:00:00Z",)"
      R"("langs":["en","fr"]})";
  std::string html = RenderLexiconSummary("app.bsky.feed.post", value);
  EXPECT_NE(html.find("hello world"), std::string::npos);
  EXPECT_NE(html.find("2026-01-01T00:00:00Z"), std::string::npos);
  EXPECT_NE(html.find("en, fr"), std::string::npos);
}

TEST(LexiconView, RendersProfileFields)
{
  const std::string value = R"({"$type":"app.bsky.actor.profile",)"
                            R"("displayName":"Alice","description":"hi"})";
  std::string html = RenderLexiconSummary("app.bsky.actor.profile", value);
  EXPECT_NE(html.find("Alice"), std::string::npos);
  EXPECT_NE(html.find("hi"), std::string::npos);
}

TEST(LexiconView, IgnoresUnknownLexicons)
{
  EXPECT_TRUE(RenderLexiconSummary("x.y.z", R"({"text":"hello"})").empty());
  EXPECT_TRUE(RenderLexiconSummary("", R"({"text":"hello"})").empty());
}

TEST(LexiconView, EscapesUntrustedValues)
{
  const std::string value = R"({"text":"<script>alert('x')</script>"})";
  std::string html = RenderLexiconSummary("app.bsky.feed.post", value);
  EXPECT_EQ(html.find("<script>"), std::string::npos);
  EXPECT_NE(html.find("&lt;script&gt;"), std::string::npos);
}

TEST(LexiconView, RendersEscapedBlobMetadataWithoutFetching)
{
  const std::string value = R"({
    "text": "photo",
    "embed": {"$type": "app.bsky.embed.images", "images": [
      {"alt": "a <b>cat</b>", "image": {"mimeType": "image/png", "size": 12345}}
    ]}
  })";
  std::string html = RenderLexiconSummary("app.bsky.feed.post", value);
  EXPECT_NE(html.find("image/png"), std::string::npos);
  EXPECT_NE(html.find("12345 bytes"), std::string::npos);
  EXPECT_NE(html.find("&lt;b&gt;cat&lt;/b&gt;"), std::string::npos);
  EXPECT_EQ(html.find("<b>cat</b>"), std::string::npos);
  EXPECT_EQ(html.find("<img"), std::string::npos);
}

TEST(LexiconView, RendersMediaFromRecordWithMedia)
{
  const std::string value = R"({
    "embed": {"$type": "app.bsky.embed.recordWithMedia", "media": {
      "images": [{"alt": "sky", "image": {"mimeType": "image/jpeg", "size": 4096}}]
    }}
  })";
  std::string html = RenderLexiconSummary("app.bsky.feed.post", value);
  EXPECT_NE(html.find("image/jpeg"), std::string::npos);
  EXPECT_NE(html.find("4096 bytes"), std::string::npos);
  EXPECT_NE(html.find("sky"), std::string::npos);
}

TEST(LexiconView, RendersProfileAvatarMetadata)
{
  const std::string value = R"({"displayName":"Alice","avatar":)"
                            R"({"mimeType":"image/webp","size":2048}})";
  std::string html = RenderLexiconSummary("app.bsky.actor.profile", value);
  EXPECT_NE(html.find("Avatar"), std::string::npos);
  EXPECT_NE(html.find("image/webp"), std::string::npos);
  EXPECT_NE(html.find("2048 bytes"), std::string::npos);
}

TEST(LexiconView, RendersBlobCidAndPdsLink)
{
  const std::string value = R"({
    "embed": {"$type": "app.bsky.embed.images", "images": [
      {"alt": "cat", "image": {"mimeType": "image/png", "size": 10,
       "ref": {"$link":
        "bafyreiba432alns2ilh32ovfgepvret2oys27j3dzxxuko3wmddnh5x5ga"}}}
    ]}
  })";
  std::string html = RenderLexiconSummary(
      "app.bsky.feed.post", value,
      LexiconViewProvenance{"did:plc:a", "https://pds.example.com"});
  EXPECT_NE(html.find("bafyreiba432alns2ilh32ovfgepvret2oys27j3dzxxuko3wmddn"
                      "h5x5ga"),
            std::string::npos);
  EXPECT_NE(html.find("href=\"https://pds.example.com/xrpc/"
                      "com.atproto.sync.getBlob?did=did%3Aplc%3Aa&amp;"
                      "cid=bafyreiba432alns2ilh32ovfgepvret2oys27j3dzxxuko3w"
                      "mddnh5x5ga\""),
            std::string::npos);
  EXPECT_NE(html.find("rel=\"noreferrer\""), std::string::npos);
  EXPECT_EQ(html.find("<img"), std::string::npos);
}

TEST(LexiconView, OmitsBlobLinkForInvalidCidOrMissingProvenance)
{
  const std::string value = R"({
    "embed": {"$type": "app.bsky.embed.images", "images": [
      {"image": {"mimeType": "image/png", "ref": {"$link": "bad cid!"}}}
    ]}
  })";
  const LexiconViewProvenance provenance{"did:plc:a",
                                         "https://pds.example.com"};
  std::string withProvenance =
      RenderLexiconSummary("app.bsky.feed.post", value, provenance);
  EXPECT_NE(withProvenance.find("bad cid!"), std::string::npos);
  EXPECT_EQ(withProvenance.find("getBlob"), std::string::npos);

  const std::string validValue = R"({
    "embed": {"$type": "app.bsky.embed.images", "images": [
      {"image": {"mimeType": "image/png", "ref": {"$link": "bafyreivalidcid"}}}
    ]}
  })";
  EXPECT_EQ(
      RenderLexiconSummary("app.bsky.feed.post", validValue).find("getBlob"),
      std::string::npos);
}

TEST(LexiconView, RendersProfileAvatarLink)
{
  const std::string value = R"({"avatar":{"mimeType":"image/webp",)"
                            R"("ref":{"$link":"bafyreivalidcid"}}})";
  std::string html = RenderLexiconSummary(
      "app.bsky.actor.profile", value,
      LexiconViewProvenance{"did:plc:a", "https://pds.example.com/"});
  EXPECT_NE(html.find("bafyreivalidcid"), std::string::npos);
  EXPECT_NE(html.find("https://pds.example.com/xrpc/com.atproto.sync.getBlob"),
            std::string::npos);
  EXPECT_NE(html.find("&amp;cid=bafyreivalidcid"), std::string::npos);
}
