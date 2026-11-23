/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "AtDocumentPolicy.h"
#include "ErrorView.h"
#include "gtest/gtest.h"

using namespace mozilla::net::atproto;

TEST(ErrorView, NamesEveryFailureKind)
{
  const FailureKind kinds[] = {
      FailureKind::HandleResolutionFailed,
      FailureKind::HandleBindingFailed,
      FailureKind::DidResolutionFailed,
      FailureKind::DidDocumentRejected,
      FailureKind::OnionEndpointUnsupported,
      FailureKind::PdsUnreachable,
      FailureKind::RecordNotFound,
      FailureKind::RecordInvalid,
      FailureKind::RepoNotFound,
      FailureKind::RepoInvalid,
      FailureKind::CollectionNotFound,
      FailureKind::CollectionInvalid,
      FailureKind::UpstreamRejected,
      FailureKind::RedirectRejected,
      FailureKind::NetworkError,
  };
  for (FailureKind kind : kinds) {
    EXPECT_NE(std::string(FailureTitle(kind)), "");
    EXPECT_NE(std::string(FailureExplanation(kind)), "");
  }
}

TEST(ErrorView, RendersExplanationAndProvenance)
{
  FailureInfo info;
  info.mKind = FailureKind::RecordNotFound;
  info.mAuthority = "alice.example";
  info.mDid = "did:plc:abc";
  info.mPds = "https://pds.example";
  info.mDetail = "HTTP 404";

  std::string html = RenderFailureHtml(info);
  EXPECT_NE(html.find(kAtDocumentCsp), std::string::npos);
  EXPECT_NE(html.find("name=\"referrer\""), std::string::npos);
  EXPECT_NE(html.find("rd-header"), std::string::npos);
  EXPECT_NE(html.find("direct at:// fetch"), std::string::npos);
  EXPECT_NE(html.find("Record not found"), std::string::npos);
  EXPECT_NE(html.find("does not exist"), std::string::npos);
  EXPECT_NE(html.find("alice.example"), std::string::npos);
  EXPECT_NE(html.find("did:plc:abc"), std::string::npos);
  EXPECT_NE(html.find("https://pds.example"), std::string::npos);
  EXPECT_NE(html.find("HTTP 404"), std::string::npos);
  EXPECT_NE(html.find("does not fall back to a gateway"), std::string::npos);
}

TEST(ErrorView, EscapesUntrustedValues)
{
  FailureInfo info;
  info.mKind = FailureKind::UpstreamRejected;
  info.mAuthority = "<script>alert('x')</script>";
  info.mDetail = "a&b\"c";

  std::string html = RenderFailureHtml(info);
  EXPECT_EQ(html.find("<script>"), std::string::npos);
  EXPECT_NE(html.find("&lt;script&gt;"), std::string::npos);
  EXPECT_NE(html.find("&amp;"), std::string::npos);
  EXPECT_NE(html.find("&quot;"), std::string::npos);
}

TEST(ErrorView, RedirectExplanationCoversOriginNotJustHost)
{
  FailureInfo info;
  info.mKind = FailureKind::RedirectRejected;
  std::string explanation = FailureExplanation(info.mKind);
  EXPECT_NE(explanation.find("origin"), std::string::npos);
  EXPECT_NE(explanation.find("scheme"), std::string::npos);
  EXPECT_NE(explanation.find("port"), std::string::npos);

  std::string html = RenderFailureHtml(info);
  EXPECT_NE(html.find("different origin"), std::string::npos);
}

TEST(ErrorView, HandleFailureExplanationCoversDnsAndHttps)
{
  std::string explanation =
      FailureExplanation(FailureKind::HandleResolutionFailed);
  EXPECT_NE(explanation.find("DNS TXT"), std::string::npos);
  EXPECT_NE(explanation.find("HTTPS"), std::string::npos);

  FailureInfo info;
  info.mKind = FailureKind::HandleResolutionFailed;
  std::string html = RenderFailureHtml(info);
  EXPECT_NE(html.find("DNS TXT"), std::string::npos);
  EXPECT_NE(html.find("HTTPS"), std::string::npos);
}

TEST(ErrorView, DistinctExplanationsPerKind)
{
  FailureInfo info;
  info.mKind = FailureKind::HandleBindingFailed;
  std::string binding = RenderFailureHtml(info);
  info.mKind = FailureKind::PdsUnreachable;
  std::string pds = RenderFailureHtml(info);
  EXPECT_NE(binding, pds);
}
