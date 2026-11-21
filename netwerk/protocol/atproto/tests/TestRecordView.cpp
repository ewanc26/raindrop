/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "RecordView.h"
#include "gtest/gtest.h"

using namespace mozilla::net::atproto;

namespace {

constexpr const char* kResponse = R"({
  "uri": "at://did:plc:abc/app.bsky.feed.post/xyz",
  "cid": "bafyreitest",
  "value": {
    "$type": "app.bsky.feed.post",
    "text": "hello",
    "replyCount": 3
  }
})";

void ExpectField(const RecordViewData& aData, const std::string& aName,
                 const std::string& aValue) {
  bool found = false;
  for (const RecordField& field : aData.mFields) {
    if (field.mName == aName) {
      found = true;
      EXPECT_EQ(field.mValue, aValue);
    }
  }
  EXPECT_TRUE(found) << aName;
}

}  // namespace

TEST(RecordView, ParsesEnvelope)
{
  RecordViewData data;
  EXPECT_EQ(ParseRecordResponse(kResponse, data), RecordViewError::None);
  EXPECT_EQ(data.mCid, "bafyreitest");
  EXPECT_EQ(data.mLexicon, "app.bsky.feed.post");
  EXPECT_EQ(data.mFields.size(), 3u);
  ExpectField(data, "$type", "\"app.bsky.feed.post\"");
  ExpectField(data, "text", "\"hello\"");
  ExpectField(data, "replyCount", "3");
  EXPECT_NE(data.mValueJson.find("\"text\""), std::string::npos);
  EXPECT_EQ(data.mRawJson, kResponse);
}

TEST(RecordView, ReportsCidVerificationStatus)
{
  RecordViewData data;
  const std::string response =
      R"({"cid":"bafyreiba432alns2ilh32ovfgepvret2oys27j3dzxxuko3wmddnh5x5ga",)"
      R"("value":{"$type":"app.bsky.feed.post","text":"hello","replyCount":3}})";
  ASSERT_EQ(ParseRecordResponse(response, data), RecordViewError::None);
  EXPECT_EQ(data.mCidStatus, CidStatus::Verified);

  RecordViewProvenance provenance{"did:plc:a", "", "https://pds", "x.y.z", "k"};
  std::string html = RenderRecordHtml(data, provenance);
  EXPECT_NE(html.find("CID verification"), std::string::npos);
  EXPECT_NE(html.find("Verified"), std::string::npos);
  EXPECT_NE(html.find("integrity only, not a repository signature"),
            std::string::npos);

  RecordViewData tampered;
  const std::string tamperedResponse =
      R"({"cid":"bafyreiba432alns2ilh32ovfgepvret2oys27j3dzxxuko3wmddnh5x5ga",)"
      R"("value":{"text":"tampered","replyCount":3}})";
  ASSERT_EQ(ParseRecordResponse(tamperedResponse, tampered),
            RecordViewError::None);
  EXPECT_EQ(tampered.mCidStatus, CidStatus::Mismatch);
}

TEST(RecordView, RejectsBadEnvelopes)
{
  RecordViewData data;
  EXPECT_EQ(ParseRecordResponse("not json", data),
            RecordViewError::InvalidJson);
  EXPECT_EQ(ParseRecordResponse("[]", data), RecordViewError::InvalidJson);
  EXPECT_EQ(ParseRecordResponse(R"({"cid":"x"})", data),
            RecordViewError::MissingValue);
  EXPECT_EQ(ParseRecordResponse(R"({"value":[]})", data),
            RecordViewError::MissingValue);
  std::string tooLong(kMaxRecordResponseLength + 1, 'a');
  EXPECT_EQ(ParseRecordResponse(tooLong, data), RecordViewError::TooLong);
}

TEST(RecordView, ReuseClearsStaleOutput)
{
  RecordViewData data;
  ASSERT_EQ(ParseRecordResponse(kResponse, data), RecordViewError::None);
  EXPECT_FALSE(data.mRawJson.empty());
  EXPECT_FALSE(data.mFields.empty());
  EXPECT_EQ(ParseRecordResponse("not json", data),
            RecordViewError::InvalidJson);
  EXPECT_TRUE(data.mRawJson.empty());
  EXPECT_TRUE(data.mValueJson.empty());
  EXPECT_TRUE(data.mLexicon.empty());
  EXPECT_TRUE(data.mFields.empty());
}

TEST(RecordView, OptionalFields)
{
  RecordViewData data;
  EXPECT_EQ(ParseRecordResponse(R"({"value":{"text":"x"}})", data),
            RecordViewError::None);
  EXPECT_TRUE(data.mCid.empty());
  EXPECT_TRUE(data.mLexicon.empty());

  std::string html =
      RenderRecordHtml(data, {"did:plc:a", "", "https://pds", "x.y.z", "k"});
  EXPECT_EQ(html.find("integrity only, not a repository signature"),
            std::string::npos);
}

TEST(RecordView, RendersProvenanceAndFields)
{
  RecordViewData data;
  ASSERT_EQ(ParseRecordResponse(kResponse, data), RecordViewError::None);
  RecordViewProvenance provenance{"did:plc:abc", "alice.example",
                                  "https://pds.example", "app.bsky.feed.post",
                                  "xyz"};
  std::string html = RenderRecordHtml(data, provenance);

  EXPECT_NE(html.find("<!DOCTYPE html>"), std::string::npos);
  EXPECT_NE(html.find("app.bsky.feed.post"), std::string::npos);
  EXPECT_NE(html.find("did:plc:abc"), std::string::npos);
  EXPECT_NE(html.find("alice.example"), std::string::npos);
  EXPECT_NE(html.find("https://pds.example"), std::string::npos);
  EXPECT_NE(html.find("bafyreitest"), std::string::npos);
  EXPECT_NE(html.find("replyCount"), std::string::npos);
  EXPECT_NE(html.find("Raw JSON"), std::string::npos);
}

TEST(RecordView, HonorsDisplayOptions)
{
  RecordViewData data;
  ASSERT_EQ(ParseRecordResponse(kResponse, data), RecordViewError::None);
  RecordViewProvenance provenance{"did:plc:abc", "alice.example",
                                  "https://pds.example", "app.bsky.feed.post",
                                  "xyz"};
  RecordViewOptions options;
  options.mShowProvenance = false;
  options.mShowRawJson = false;

  std::string html = RenderRecordHtml(data, provenance, options);
  EXPECT_EQ(html.find("Provenance"), std::string::npos);
  EXPECT_EQ(html.find("alice.example"), std::string::npos);
  EXPECT_EQ(html.find("Raw JSON"), std::string::npos);
  EXPECT_NE(html.find("<h2>Fields</h2>"), std::string::npos);
}

TEST(RecordView, ShowsHandleDurabilityHintOnlyForHandles)
{
  RecordViewData data;
  ASSERT_EQ(ParseRecordResponse(R"({"value":{"$type":"x.y.z"}})", data),
            RecordViewError::None);

  std::string handleHtml = RenderRecordHtml(
      data, {"did:plc:abc", "alice.example", "https://pds", "x.y.z", "k"});
  EXPECT_NE(handleHtml.find("may change"), std::string::npos);
  EXPECT_NE(handleHtml.find("resolved to did:plc:abc"), std::string::npos);

  std::string didHtml =
      RenderRecordHtml(data, {"did:plc:abc", "", "https://pds", "x.y.z", "k"});
  EXPECT_EQ(didHtml.find("may change"), std::string::npos);
}

TEST(RecordView, PassesProvenanceToBlobLinks)
{
  const std::string response =
      R"({"value":{"$type":"app.bsky.feed.post","text":"photo",)"
      R"("embed":{"$type":"app.bsky.embed.images","images":[)"
      R"({"image":{"mimeType":"image/png",)"
      R"("ref":{"$link":"bafyreivalidcid"}}}]}}})";
  RecordViewData data;
  ASSERT_EQ(ParseRecordResponse(response, data), RecordViewError::None);
  RecordViewProvenance provenance{"did:plc:a", "", "https://pds.example",
                                  "app.bsky.feed.post", "k"};

  std::string html = RenderRecordHtml(data, provenance);
  EXPECT_NE(html.find("bafyreivalidcid"), std::string::npos);
  EXPECT_NE(html.find("https://pds.example/xrpc/com.atproto.sync.getBlob"),
            std::string::npos);
}

TEST(RecordView, EscapesHtmlInRecordData)
{
  const std::string response =
      R"({"cid":"x","value":{"$type":"x.y.z","text":"<script>alert('x')</script>&\""}})";
  RecordViewData data;
  ASSERT_EQ(ParseRecordResponse(response, data), RecordViewError::None);
  RecordViewProvenance provenance{"did:plc:a", "", "https://pds", "x.y.z", "k"};
  std::string html = RenderRecordHtml(data, provenance);

  EXPECT_EQ(html.find("<script>"), std::string::npos);
  EXPECT_NE(html.find("&lt;script&gt;"), std::string::npos);
  EXPECT_NE(html.find("&amp;"), std::string::npos);
  EXPECT_NE(html.find("&quot;"), std::string::npos);
  EXPECT_NE(html.find("&#39;"), std::string::npos);
}
