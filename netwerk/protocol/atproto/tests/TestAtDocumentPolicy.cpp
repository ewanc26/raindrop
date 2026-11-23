/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "AtDocumentPolicy.h"
#include "gtest/gtest.h"

using namespace mozilla::net::atproto;

TEST(AtDocumentPolicy, MetaCarriesGeneratedDocumentPolicy)
{
  std::string html;
  AppendDocumentSecurityMeta(html);

  EXPECT_NE(html.find("<meta http-equiv=\"Content-Security-Policy\""),
            std::string::npos);
  EXPECT_NE(html.find(kAtDocumentCsp), std::string::npos);
  EXPECT_NE(html.find("<meta name=\"referrer\" content=\"no-referrer\">"),
            std::string::npos);
}

TEST(AtDocumentPolicy, PolicyBlocksScriptingAndFraming)
{
  EXPECT_NE(std::string(kAtDocumentCsp).find("default-src 'none'"),
            std::string::npos);
  EXPECT_NE(std::string(kAtDocumentCsp).find("script-src 'none'"),
            std::string::npos);
  EXPECT_NE(std::string(kAtDocumentCsp).find("frame-ancestors 'none'"),
            std::string::npos);
  EXPECT_NE(std::string(kAtDocumentCsp).find("img-src https: data:"),
            std::string::npos);
}

TEST(AtDocumentPolicy, IconIsLocalSelfContainedDataUri)
{
  std::string html;
  AppendDocumentIcon(html);

  EXPECT_EQ(std::string(kAtDocumentIcon).rfind("data:image/svg+xml;base64,", 0),
            0u);
  EXPECT_NE(html.find("<link rel=\"icon\" type=\"image/svg+xml\" href=\""),
            std::string::npos);
  EXPECT_NE(html.find("data:image/svg+xml;base64,"), std::string::npos);
  EXPECT_EQ(html.find("href=\"http"), std::string::npos);
  EXPECT_EQ(html.find("href=\"//"), std::string::npos);
}

TEST(AtDocumentPolicy, StyleDefaultsFollowSystem)
{
  std::string css;
  AppendDocumentStyle(css);

  EXPECT_NE(css.find("<style>\n"), std::string::npos);
  EXPECT_NE(css.find("color-scheme:light dark"), std::string::npos);
  EXPECT_NE(css.find("@media(prefers-color-scheme:dark)"), std::string::npos);
  EXPECT_NE(css.find("--rd-font-size:14px"), std::string::npos);
  EXPECT_NE(css.find("--rd-max-width:60em"), std::string::npos);
  EXPECT_NE(css.find("--rd-margin:2em"), std::string::npos);
  EXPECT_EQ(css.find("</style>\n"), css.rfind("</style>\n"));
}

TEST(AtDocumentPolicy, ExplicitThemesOverrideSystemPreference)
{
  ViewStyleOptions dark;
  dark.mTheme = ViewTheme::Dark;
  std::string css;
  AppendDocumentStyle(css, dark);

  EXPECT_NE(css.find(":root{color-scheme:dark;"), std::string::npos);
  EXPECT_NE(css.find("--rd-bg:#181a1b"), std::string::npos);
  EXPECT_NE(css.find("--rd-fg:#eeeeee"), std::string::npos);

  ViewStyleOptions light;
  light.mTheme = ViewTheme::Light;
  css.clear();
  AppendDocumentStyle(css, light);
  EXPECT_NE(css.find(":root{color-scheme:light;"), std::string::npos);
  EXPECT_NE(css.find("--rd-bg:#ffffff"), std::string::npos);
}

TEST(AtDocumentPolicy, HighContrastUsesStrongColours)
{
  ViewStyleOptions options;
  options.mTheme = ViewTheme::HighContrast;
  std::string css;
  AppendDocumentStyle(css, options);

  EXPECT_NE(css.find("--rd-bg:#000000"), std::string::npos);
  EXPECT_NE(css.find("--rd-fg:#ffffff"), std::string::npos);
  EXPECT_NE(css.find("--rd-link:#ffff00"), std::string::npos);
  EXPECT_NE(css.find("a{text-decoration:underline}"), std::string::npos);
}

TEST(AtDocumentPolicy, SizeWidthDensityAndAccentAreIndependent)
{
  ViewStyleOptions options;
  options.mTextSize = ViewTextSize::ExtraLarge;
  options.mWidth = ViewWidth::Full;
  options.mCompact = true;
  options.mAccent = ViewAccent::Amber;
  std::string css;
  AppendDocumentStyle(css, options);

  EXPECT_NE(css.find("--rd-font-size:20px"), std::string::npos);
  EXPECT_NE(css.find("--rd-max-width:none"), std::string::npos);
  EXPECT_NE(css.find("--rd-margin:1em"), std::string::npos);
  EXPECT_NE(css.find("--rd-cell-padding:.2em .4em"), std::string::npos);
  EXPECT_NE(css.find("--rd-link:#a1651d"), std::string::npos);
  EXPECT_NE(css.find("--rd-link:#fdd663"), std::string::npos);
  EXPECT_EQ(css.find("--rd-font-size:14px"), std::string::npos);
}

TEST(AtDocumentPolicy, StyleContainsNoScriptingOrRemoteResources)
{
  std::string css;
  AppendDocumentStyle(css);

  EXPECT_EQ(css.find("<script"), std::string::npos);
  EXPECT_EQ(css.find("url("), std::string::npos);
  EXPECT_EQ(css.find("http://"), std::string::npos);
  EXPECT_EQ(css.find("https://"), std::string::npos);
  EXPECT_NE(css.find(".rd-header"), std::string::npos);
  EXPECT_NE(css.find(".rd-mark"), std::string::npos);
}

TEST(AtDocumentPolicy, HeaderCarriesIdentityAndEscapedProvenance)
{
  DocumentHeader header;
  header.mRequested = "alice.example";
  header.mDid = "did:plc:abc";
  header.mPds = "https://pds.example";
  header.mCollection = "app.bsky.feed.post";
  header.mRecordKey = "3l<script>";
  header.mCid = "bafyreitest";
  header.mCidStatus = "Verified";

  std::string html;
  AppendDocumentHeader(html, header);

  EXPECT_NE(html.find("raindrop"), std::string::npos);
  EXPECT_NE(html.find("direct at:// fetch"), std::string::npos);
  EXPECT_NE(html.find("alice.example"), std::string::npos);
  EXPECT_NE(html.find("did:plc:abc"), std::string::npos);
  EXPECT_NE(html.find("https://pds.example"), std::string::npos);
  EXPECT_NE(html.find("app.bsky.feed.post"), std::string::npos);
  EXPECT_NE(html.find("3l&lt;script&gt;"), std::string::npos);
  EXPECT_NE(html.find("bafyreitest"), std::string::npos);
  EXPECT_NE(html.find("Verified"), std::string::npos);
  EXPECT_EQ(html.find("3l<script>"), std::string::npos);
  EXPECT_EQ(html.find("<script"), std::string::npos);
  EXPECT_EQ(html.find("<img"), std::string::npos);
  EXPECT_EQ(html.find("href="), std::string::npos);
}

TEST(AtDocumentPolicy, HeaderSkipsRepeatedProvenanceSteps)
{
  DocumentHeader header;
  header.mRequested = "did:plc:abc";
  header.mDid = "did:plc:abc";
  std::string html;
  AppendDocumentHeader(html, header);

  size_t first = html.find("did:plc:abc");
  EXPECT_NE(first, std::string::npos);
  EXPECT_EQ(html.find("did:plc:abc", first + 1), std::string::npos);
}

TEST(AtDocumentPolicy, HeaderFallsBackToDirectMode)
{
  std::string html;
  AppendDocumentHeader(html, {});

  EXPECT_NE(html.find("Mode"), std::string::npos);
  EXPECT_NE(html.find("direct"), std::string::npos);
}
