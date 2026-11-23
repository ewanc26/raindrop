/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "AtDocumentPolicy.h"
#include "RepoView.h"
#include "gtest/gtest.h"

using namespace mozilla::net::atproto;

namespace {

constexpr const char* kRepoJson =
    R"({"did":"did:plc:abc","handle":"alice.example",)"
    R"("collections":["app.bsky.feed.post","app.bsky.actor.profile",42]})";

}  // namespace

TEST(RepoView, ParsesIdentityAndCollections)
{
  RepoViewData data;
  EXPECT_EQ(ParseRepoResponse(kRepoJson, data), RepoViewError::None);
  EXPECT_EQ(data.mDid, "did:plc:abc");
  EXPECT_EQ(data.mHandle, "alice.example");
  ASSERT_EQ(data.mCollections.size(), size_t(2));
  EXPECT_EQ(data.mCollections[0], "app.bsky.feed.post");
  EXPECT_EQ(data.mCollections[1], "app.bsky.actor.profile");
}

TEST(RepoView, HandleAndCollectionsAreOptional)
{
  RepoViewData data;
  EXPECT_EQ(ParseRepoResponse(R"({"did":"did:web:user.example.com"})", data),
            RepoViewError::None);
  EXPECT_EQ(data.mDid, "did:web:user.example.com");
  EXPECT_TRUE(data.mHandle.empty());
  EXPECT_TRUE(data.mCollections.empty());
}

TEST(RepoView, RejectsInvalidResponses)
{
  RepoViewData data;
  EXPECT_EQ(ParseRepoResponse("not json", data), RepoViewError::InvalidJson);
  EXPECT_EQ(ParseRepoResponse("[]", data), RepoViewError::InvalidJson);
  EXPECT_EQ(ParseRepoResponse(R"({"handle":"alice.example"})", data),
            RepoViewError::MissingDid);
  EXPECT_EQ(
      ParseRepoResponse(std::string(kMaxRepoResponseLength + 1, 'x'), data),
      RepoViewError::TooLong);
}

TEST(RepoView, RendersSummaryAndEscapesUntrustedValues)
{
  RepoViewData data;
  ASSERT_EQ(ParseRepoResponse(kRepoJson, data), RepoViewError::None);
  data.mHandle = "<script>alert('x')</script>";

  std::string html =
      RenderRepoHtml(data, RepoViewProvenance{"did:plc:abc", "alice.example",
                                              "https://pds.example.com"});
  EXPECT_NE(html.find(kAtDocumentCsp), std::string::npos);
  EXPECT_NE(html.find("name=\"referrer\""), std::string::npos);
  EXPECT_NE(html.find("rel=\"icon\""), std::string::npos);
  EXPECT_NE(html.find("data:image/svg+xml;base64,"), std::string::npos);
  EXPECT_NE(html.find("rd-header"), std::string::npos);
  EXPECT_NE(html.find("direct at:// fetch"), std::string::npos);
  EXPECT_NE(html.find("Repository summary"), std::string::npos);
  EXPECT_NE(html.find("did:plc:abc"), std::string::npos);
  EXPECT_NE(html.find("https://pds.example.com"), std::string::npos);
  EXPECT_NE(html.find("app.bsky.feed.post"), std::string::npos);
  EXPECT_NE(html.find("No gateway was contacted"), std::string::npos);
  EXPECT_EQ(html.find("<script>"), std::string::npos);
  EXPECT_NE(html.find("&lt;script&gt;"), std::string::npos);
  EXPECT_NE(html.find("&#39;"), std::string::npos);
}

TEST(RepoView, ShowsEmptyCollectionList)
{
  RepoViewData data;
  ASSERT_EQ(ParseRepoResponse(R"({"did":"did:plc:abc"})", data),
            RepoViewError::None);
  std::string html =
      RenderRepoHtml(data, RepoViewProvenance{"did:plc:abc", "", ""});
  EXPECT_NE(html.find("(none advertised)"), std::string::npos);
}

TEST(RepoView, AppliesSuppliedStyle)
{
  RepoViewData data;
  ASSERT_EQ(ParseRepoResponse(R"({"did":"did:plc:abc"})", data),
            RepoViewError::None);
  ViewStyleOptions style;
  style.mTheme = ViewTheme::HighContrast;
  style.mTextSize = ViewTextSize::Large;
  std::string html =
      RenderRepoHtml(data, RepoViewProvenance{"did:plc:abc", "", ""}, style);

  EXPECT_NE(html.find("--rd-bg:#000000"), std::string::npos);
  EXPECT_NE(html.find("--rd-font-size:17px"), std::string::npos);
  EXPECT_NE(html.find("Repository summary"), std::string::npos);
  EXPECT_EQ(html.find("<style>"), html.rfind("<style>"));
}
