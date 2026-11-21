/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "PrivacyFlags.h"

#include "gtest/gtest.h"
#include "nsIRequest.h"

using namespace mozilla::net::atproto;

TEST(PrivacyFlags, InhibitsDiskCacheAndBypassesCache) {
  uint32_t flags = ResolutionLoadFlags(nsIRequest::LOAD_NORMAL);
  EXPECT_TRUE(flags & nsIRequest::INHIBIT_PERSISTENT_CACHING);
  EXPECT_TRUE(flags & nsIRequest::LOAD_BYPASS_CACHE);
}

TEST(PrivacyFlags, PreservesExistingFlags) {
  const uint32_t base = nsIRequest::VALIDATE_ALWAYS | nsIRequest::LOAD_ANONYMOUS;
  uint32_t flags = ResolutionLoadFlags(base);
  EXPECT_TRUE(flags & nsIRequest::VALIDATE_ALWAYS);
  EXPECT_TRUE(flags & nsIRequest::LOAD_ANONYMOUS);
  EXPECT_TRUE(flags & nsIRequest::INHIBIT_PERSISTENT_CACHING);
  EXPECT_TRUE(flags & nsIRequest::LOAD_BYPASS_CACHE);
}
