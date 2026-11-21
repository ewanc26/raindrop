/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "LexiconView.h"

#include <string>

#include "gtest/gtest.h"

using namespace mozilla::net::atproto;

TEST(LexiconView, RendersPostTextAndDate) {
  const std::string value = R"({"$type":"app.bsky.feed.post",)"
                            R"("text":"hello world","createdAt":"2026-01-01T00:00:00Z",)"
                            R"("langs":["en","fr"]})";
  std::string html = RenderLexiconSummary("app.bsky.feed.post", value);
  EXPECT_NE(html.find("hello world"), std::string::npos);
  EXPECT_NE(html.find("2026-01-01T00:00:00Z"), std::string::npos);
  EXPECT_NE(html.find("en, fr"), std::string::npos);
}

TEST(LexiconView, RendersProfileFields) {
  const std::string value = R"({"$type":"app.bsky.actor.profile",)"
                            R"("displayName":"Alice","description":"hi"})";
  std::string html = RenderLexiconSummary("app.bsky.actor.profile", value);
  EXPECT_NE(html.find("Alice"), std::string::npos);
  EXPECT_NE(html.find("hi"), std::string::npos);
}

TEST(LexiconView, IgnoresUnknownLexicons) {
  EXPECT_TRUE(RenderLexiconSummary("x.y.z", R"({"text":"hello"})").empty());
  EXPECT_TRUE(RenderLexiconSummary("", R"({"text":"hello"})").empty());
}

TEST(LexiconView, EscapesUntrustedValues) {
  const std::string value = R"({"text":"<script>alert('x')</script>"})";
  std::string html = RenderLexiconSummary("app.bsky.feed.post", value);
  EXPECT_EQ(html.find("<script>"), std::string::npos);
  EXPECT_NE(html.find("&lt;script&gt;"), std::string::npos);
}
