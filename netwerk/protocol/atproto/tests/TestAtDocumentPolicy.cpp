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
}
