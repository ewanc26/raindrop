/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <string_view>
#include <vector>

#include "LexiconView.h"
#include "gtest/gtest.h"
#include "json/json.h"

using namespace mozilla::net::atproto;

namespace {

std::string MakeFacetPost(std::string_view aText, size_t aStart, size_t aEnd,
                          std::string_view aType, std::string_view aKey,
                          std::string_view aValue) {
  Json::Value root;
  root["text"] = std::string(aText);
  Json::Value facet;
  facet["index"]["byteStart"] = static_cast<Json::UInt64>(aStart);
  facet["index"]["byteEnd"] = static_cast<Json::UInt64>(aEnd);
  facet["feature"]["$type"] = std::string(aType);
  facet["feature"][std::string(aKey)] = std::string(aValue);
  root["facets"].append(facet);
  Json::StreamWriterBuilder builder;
  builder["indentation"] = "";
  return Json::writeString(builder, root);
}

}  // namespace

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
      LexiconViewProvenance{"did:plc:a", "https://pds.example.com", false});
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

TEST(LexiconView, RendersReplyAndQuoteReferences)
{
  const std::string value = R"({
    "text": "hello",
    "reply": {
      "parent": {"uri": "at://did:plc:a/app.bsky.feed.post/parent"},
      "root": {"uri": "at://did:plc:a/app.bsky.feed.post/root"}
    },
    "embed": {"$type": "app.bsky.embed.record", "record": {
      "uri": "at://did:plc:b/app.bsky.feed.post/quote"
    }}
  })";
  std::string html = RenderLexiconSummary("app.bsky.feed.post", value);
  EXPECT_NE(html.find("References:"), std::string::npos);
  EXPECT_NE(html.find("href=\"at://did:plc:a/app.bsky.feed.post/parent\""),
            std::string::npos);
  EXPECT_NE(html.find("href=\"at://did:plc:a/app.bsky.feed.post/root\""),
            std::string::npos);
  EXPECT_NE(html.find("href=\"at://did:plc:b/app.bsky.feed.post/quote\""),
            std::string::npos);
}

TEST(LexiconView, IgnoresInvalidReferenceUris)
{
  const std::string value = R"({
    "reply": {
      "parent": {"uri": "https://example.com/post"},
      "root": {"uri": "javascript:alert"}
    },
    "embed": {"$type": "app.bsky.embed.record", "record": {
      "uri": "at://did:plc:a/app.bsky.feed.post/bad key"
    }}
  })";
  std::string html = RenderLexiconSummary("app.bsky.feed.post", value);
  EXPECT_EQ(html.find("<a href="), std::string::npos);
  EXPECT_EQ(html.find("References:"), std::string::npos);
}

TEST(LexiconView, RejectsFragmentedReferenceUri)
{
  const std::string value = R"({
    "reply": {"parent": {
      "uri": "at://did:plc:a/app.bsky.feed.post/self#/%ff"
    }}
  })";
  std::string html = RenderLexiconSummary("app.bsky.feed.post", value);
  EXPECT_EQ(html.find("<a href="), std::string::npos);
}

TEST(LexiconView, RendersRecordWithMediaReference)
{
  const std::string value = R"({
    "embed": {"$type": "app.bsky.embed.recordWithMedia",
      "record": {"uri": "at://did:plc:a/app.bsky.feed.post/quoted"},
      "media": {"$type": "app.bsky.embed.images", "images": [
        {"image": {"mimeType": "image/png", "ref": {"$link": "bafyreicid"}}}
      ]}
    }
  })";
  std::string html = RenderLexiconSummary("app.bsky.feed.post", value);
  EXPECT_NE(html.find("href=\"at://did:plc:a/app.bsky.feed.post/quoted\""),
            std::string::npos);
  EXPECT_NE(html.find("image/png"), std::string::npos);
}

TEST(LexiconView, RendersInlineImageWithEscapedAlt)
{
  const std::string value = R"({
    "embed": {"$type": "app.bsky.embed.images", "images": [
      {"alt": "a <b>cat</b>", "image": {"mimeType": "image/jpeg",
       "ref": {"$link": "bafyreivalidcid"}}}
    ]}
  })";
  std::string html = RenderLexiconSummary(
      "app.bsky.feed.post", value,
      LexiconViewProvenance{"did:plc:a", "https://pds.example.com", true});
  EXPECT_NE(html.find("<img class=\"blob\" "
                      "src=\"https://pds.example.com/xrpc/"
                      "com.atproto.sync.getBlob?did=did%3Aplc%3Aa&amp;"
                      "cid=bafyreivalidcid\""),
            std::string::npos);
  EXPECT_NE(html.find("alt=\"a &lt;b&gt;cat&lt;/b&gt;\""), std::string::npos);
  EXPECT_NE(html.find("loading=\"lazy\""), std::string::npos);
}

TEST(LexiconView, OmitsInlineImageWhenDisabled)
{
  const std::string value = R"({
    "embed": {"$type": "app.bsky.embed.images", "images": [
      {"image": {"mimeType": "image/png", "ref": {"$link": "bafyreicid"}}}
    ]}
  })";
  std::string html = RenderLexiconSummary(
      "app.bsky.feed.post", value,
      LexiconViewProvenance{"did:plc:a", "https://pds.example.com", false});
  EXPECT_EQ(html.find("<img"), std::string::npos);
  EXPECT_NE(html.find("getBlob"), std::string::npos);
}

TEST(LexiconView, DefaultsToCidLinkWithoutInlineFetch)
{
  const std::string value = R"({
    "embed": {"$type": "app.bsky.embed.images", "images": [
      {"image": {"mimeType": "image/png", "ref": {"$link": "bafyreivalidcid"}}}
    ]}
  })";
  std::string html = RenderLexiconSummary(
      "app.bsky.feed.post", value,
      LexiconViewProvenance{"did:plc:a", "https://pds.example.com"});
  EXPECT_EQ(html.find("<img"), std::string::npos);
  EXPECT_NE(html.find("bafyreivalidcid"), std::string::npos);
  EXPECT_NE(html.find("getBlob"), std::string::npos);
}

TEST(LexiconView, OmitsInlineImageForNonImageMime)
{
  const std::string value = R"({
    "embed": {"$type": "app.bsky.embed.images", "images": [
      {"image": {"mimeType": "application/octet-stream",
       "ref": {"$link": "bafyreicid"}}}
    ]}
  })";
  std::string html = RenderLexiconSummary(
      "app.bsky.feed.post", value,
      LexiconViewProvenance{"did:plc:a", "https://pds.example.com"});
  EXPECT_EQ(html.find("<img"), std::string::npos);
  EXPECT_NE(html.find("getBlob"), std::string::npos);
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

TEST(LexiconView, RendersLikeSubjectLink)
{
  const std::string value =
      R"({"createdAt":"2026-01-01T00:00:00Z",)"
      R"("subject":{"uri":"at://did:plc:b/app.bsky.feed.post/k"}})";
  std::string html = RenderLexiconSummary("app.bsky.feed.like", value);
  EXPECT_FALSE(html.empty());
  EXPECT_NE(html.find("2026-01-01T00:00:00Z"), std::string::npos);
  EXPECT_NE(html.find("Liked"), std::string::npos);
  EXPECT_NE(html.find("href=\"at://did:plc:b/app.bsky.feed.post/k\""),
            std::string::npos);
}

TEST(LexiconView, RendersRepostSubjectLink)
{
  const std::string value =
      R"({"subject":{"uri":"at://did:plc:c/app.bsky.feed.post/r"}})";
  std::string html = RenderLexiconSummary("app.bsky.feed.repost", value);
  EXPECT_FALSE(html.empty());
  EXPECT_NE(html.find("Reposted"), std::string::npos);
  EXPECT_NE(html.find("href=\"at://did:plc:c/app.bsky.feed.post/r\""),
            std::string::npos);
}

TEST(LexiconView, RendersFollowSubjectLink)
{
  const std::string value = R"({"subject":"did:plc:d"})";
  std::string html = RenderLexiconSummary("app.bsky.graph.follow", value);
  EXPECT_FALSE(html.empty());
  EXPECT_NE(html.find("Follows"), std::string::npos);
  EXPECT_NE(html.find("href=\"at://did:plc:d\""), std::string::npos);
}

TEST(LexiconView, IgnoresInvalidSocialSubjects)
{
  const std::string like = R"({"subject":{"uri":"https://example.com/post"}})";
  const std::string repost = R"({"subject":{"uri":"at://did:plc:x/bad key"}})";
  const std::string follow = R"({"subject":"not-a-did"})";
  EXPECT_EQ(RenderLexiconSummary("app.bsky.feed.like", like).find("<a href="),
            std::string::npos);
  EXPECT_EQ(
      RenderLexiconSummary("app.bsky.feed.repost", repost).find("<a href="),
      std::string::npos);
  EXPECT_EQ(
      RenderLexiconSummary("app.bsky.graph.follow", follow).find("<a href="),
      std::string::npos);
}

TEST(LexiconView, EscapesSocialActionValues)
{
  const std::string value = R"({"createdAt":"<script>alert('x')</script>"})";
  std::string html = RenderLexiconSummary("app.bsky.feed.like", value);
  EXPECT_EQ(html.find("<script>"), std::string::npos);
  EXPECT_NE(html.find("&lt;script&gt;"), std::string::npos);
}

TEST(LexiconView, RendersLinkFacetAsSafeAnchor)
{
  const std::string url = "https://example.com/a?x=1&y=2";
  const std::string text = "See " + url + " now";
  const size_t start = text.find(url);
  std::string html = RenderLexiconSummary(
      "app.bsky.feed.post",
      MakeFacetPost(text, start, start + url.size(),
                    "app.bsky.richtext.facet#link", "uri", url));
  EXPECT_NE(html.find("href=\"https://example.com/a?x=1&amp;y=2\""),
            std::string::npos);
  EXPECT_NE(html.find("rel=\"noopener noreferrer\""), std::string::npos);
  EXPECT_NE(html.find(">https://example.com/a?x=1&amp;y=2</a>"),
            std::string::npos);
  EXPECT_NE(html.find("<td>See "), std::string::npos);
  EXPECT_NE(html.find(" now</td>"), std::string::npos);
}

TEST(LexiconView, RejectsUnsafeLinkFacets)
{
  const std::vector<std::string> badUrls = {
      "javascript:alert(1)",
      "data:text/html,<script>alert(1)</script>",
      "file:///etc/passwd",
      "https://user@example.com/",
      "http://",
      "https:///example.com",
      "https://example.com:abc/",
      "https://example.com/a b",
      "https://example.com/\\evil",
  };
  for (const std::string& url : badUrls) {
    std::string html = RenderLexiconSummary(
        "app.bsky.feed.post",
        MakeFacetPost(url, 0, url.size(), "app.bsky.richtext.facet#link", "uri",
                      url));
    EXPECT_EQ(html.find("<a href="), std::string::npos) << url;
    EXPECT_NE(html.find("<td>"), std::string::npos) << url;
  }

  const std::string text = "hello";
  EXPECT_EQ(RenderLexiconSummary(
                "app.bsky.feed.post",
                MakeFacetPost(text, 0, 99, "app.bsky.richtext.facet#link",
                              "uri", "https://example.com"))
                .find("<a href="),
            std::string::npos);
  EXPECT_EQ(RenderLexiconSummary(
                "app.bsky.feed.post",
                MakeFacetPost(text, 4, 1, "app.bsky.richtext.facet#link", "uri",
                              "https://example.com"))
                .find("<a href="),
            std::string::npos);
}

TEST(LexiconView, RendersMentionFacetOnlyForValidDid)
{
  const std::string did = "did:plc:vwzwgnygau7ed7b7wt5ux7y2";
  const std::string text = "@" + did;
  std::string html = RenderLexiconSummary(
      "app.bsky.feed.post",
      MakeFacetPost(text, 0, text.size(), "app.bsky.richtext.facet#mention",
                    "did", did));
  EXPECT_NE(html.find("href=\"at://did:plc:vwzwgnygau7ed7b7wt5ux7y2\""),
            std::string::npos);

  const std::string invalid = "@not-a-did";
  std::string rejected = RenderLexiconSummary(
      "app.bsky.feed.post",
      MakeFacetPost(invalid, 0, invalid.size(),
                    "app.bsky.richtext.facet#mention", "did", "javascript:1"));
  EXPECT_EQ(rejected.find("<a href="), std::string::npos);
  EXPECT_NE(rejected.find("@not-a-did"), std::string::npos);
}

TEST(LexiconView, RendersTagFacetAsEscapedLocalText)
{
  const std::string text = "topic #Raindrop now";
  std::string html = RenderLexiconSummary(
      "app.bsky.feed.post",
      MakeFacetPost(text, 6, 15, "app.bsky.richtext.facet#tag", "tag",
                    "Raindrop"));
  EXPECT_NE(html.find("<span class=\"hint\">#Raindrop</span>"),
            std::string::npos);
  EXPECT_EQ(html.find("href="), std::string::npos);

  const std::string hostileTag = "<img src=x onerror=alert(1)>";
  std::string hostile = RenderLexiconSummary(
      "app.bsky.feed.post",
      MakeFacetPost("#x", 0, 2, "app.bsky.richtext.facet#tag", "tag",
                    hostileTag));
  EXPECT_EQ(hostile.find("<img"), std::string::npos);
  EXPECT_NE(hostile.find("<span class=\"hint\">#x</span>"), std::string::npos);

  std::string control = RenderLexiconSummary(
      "app.bsky.feed.post",
      MakeFacetPost("#x", 0, 2, "app.bsky.richtext.facet#tag", "tag",
                    std::string("\x01"
                                "x")));
  EXPECT_EQ(control.find("<span class=\"hint\">"), std::string::npos);
}

TEST(LexiconView, ClampsFacetsToUtf8Boundaries)
{
  const std::string text =
      "\xc3\xa9"
      "\xf0\x9f\x98\x80";
  const std::string url = "https://example.com";
  std::string split = RenderLexiconSummary(
      "app.bsky.feed.post",
      MakeFacetPost(text, 1, 4, "app.bsky.richtext.facet#link", "uri", url));
  EXPECT_EQ(split.find("<a href="), std::string::npos);

  std::string clamped = RenderLexiconSummary(
      "app.bsky.feed.post",
      MakeFacetPost(text, 1, 6, "app.bsky.richtext.facet#link", "uri", url));
  EXPECT_NE(clamped.find(">\xf0\x9f\x98\x80</a>"), std::string::npos);
  EXPECT_EQ(clamped.find("\xef\xbf\xbd"), std::string::npos);
}

TEST(LexiconView, DropsOverlappingAndUnknownFacets)
{
  const std::string value = R"({"text":"abcdef","facets":[
    {"index":{"byteStart":0,"byteEnd":4},
     "feature":{"$type":"app.bsky.richtext.facet#link",
                "uri":"https://first.example"}},
    {"index":{"byteStart":2,"byteEnd":6},
     "feature":{"$type":"app.bsky.richtext.facet#link",
                "uri":"https://second.example"}},
    {"index":{"byteStart":4,"byteEnd":6},
     "feature":{"$type":"x.y.unknown","uri":"https://third.example"}}
  ]})";
  std::string html = RenderLexiconSummary("app.bsky.feed.post", value);
  EXPECT_NE(html.find("https://first.example"), std::string::npos);
  EXPECT_EQ(html.find("https://second.example"), std::string::npos);
  EXPECT_EQ(html.find("https://third.example"), std::string::npos);
  EXPECT_NE(html.find("ef</td>"), std::string::npos);
}
