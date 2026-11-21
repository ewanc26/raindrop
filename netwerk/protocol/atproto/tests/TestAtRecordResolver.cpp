/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "AtRecordResolver.h"

#include "gtest/gtest.h"

using namespace mozilla::net::atproto;

TEST(AtRecordResolver, SameOriginAllowsDefaultPortSpelling) {
  EXPECT_TRUE(IsSameOrigin("https", "pds.example.com", -1,
                           "https", "pds.example.com", 443));
}

TEST(AtRecordResolver, SameOriginIgnoresAsciiCase) {
  EXPECT_TRUE(IsSameOrigin("https", "PDS.Example.COM", -1,
                           "HTTPS", "pds.example.com", -1));
}

TEST(AtRecordResolver, RejectsSchemeDowngrade) {
  EXPECT_FALSE(IsSameOrigin("https", "pds.example.com", -1,
                            "http", "pds.example.com", -1));
}

TEST(AtRecordResolver, RejectsPortChange) {
  EXPECT_FALSE(IsSameOrigin("https", "pds.example.com", -1,
                            "https", "pds.example.com", 8443));
}

TEST(AtRecordResolver, RejectsHostChange) {
  EXPECT_FALSE(IsSameOrigin("https", "pds.example.com", -1,
                            "https", "evil.example.com", -1));
}
