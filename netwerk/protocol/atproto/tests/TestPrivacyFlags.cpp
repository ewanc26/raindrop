/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "PrivacyFlags.h"
#include "gtest/gtest.h"
#include "mozilla/OriginAttributes.h"
#include "nsComponentManagerUtils.h"
#include "nsContentUtils.h"
#include "nsIHttpChannel.h"
#include "nsILoadInfo.h"
#include "nsIReferrerInfo.h"
#include "nsIRequest.h"
#include "nsIURI.h"
#include "nsNetUtil.h"

using namespace mozilla::net::atproto;

TEST(PrivacyFlags, InhibitsDiskCacheAndBypassesCache)
{
  uint32_t flags = ResolutionLoadFlags(nsIRequest::LOAD_NORMAL);
  EXPECT_TRUE(flags & nsIRequest::INHIBIT_PERSISTENT_CACHING);
  EXPECT_TRUE(flags & nsIRequest::LOAD_BYPASS_CACHE);
}

TEST(PrivacyFlags, PreservesExistingFlags)
{
  const uint32_t base =
      nsIRequest::VALIDATE_ALWAYS | nsIRequest::LOAD_ANONYMOUS;
  uint32_t flags = ResolutionLoadFlags(base);
  EXPECT_TRUE(flags & nsIRequest::VALIDATE_ALWAYS);
  EXPECT_TRUE(flags & nsIRequest::LOAD_ANONYMOUS);
  EXPECT_TRUE(flags & nsIRequest::INHIBIT_PERSISTENT_CACHING);
  EXPECT_TRUE(flags & nsIRequest::LOAD_BYPASS_CACHE);
}

TEST(PrivacyFlags, PrepareResolutionFetchClearsReferrerAndCopiesOriginState)
{
  nsCOMPtr<nsIURI> uri;
  ASSERT_TRUE(
      NS_SUCCEEDED(NS_NewURI(getter_AddRefs(uri), "https://pds.example/x"_ns)));

  nsCOMPtr<nsIChannel> origin;
  ASSERT_TRUE(NS_SUCCEEDED(NS_NewChannel(
      getter_AddRefs(origin), uri, nsContentUtils::GetSystemPrincipal(),
      nsILoadInfo::SEC_ALLOW_CROSS_ORIGIN_SEC_CONTEXT_IS_NULL,
      nsIContentPolicy::TYPE_OTHER)));
  nsCOMPtr<nsILoadInfo> originLoadInfo;
  ASSERT_TRUE(
      NS_SUCCEEDED(origin->GetLoadInfo(getter_AddRefs(originLoadInfo))));
  mozilla::OriginAttributes oa = originLoadInfo->GetOriginAttributes();
  oa.mPrivateBrowsingId = 1;
  originLoadInfo->SetOriginAttributes(oa);

  nsCOMPtr<nsIChannel> fetch;
  ASSERT_TRUE(NS_SUCCEEDED(NS_NewChannel(
      getter_AddRefs(fetch), uri, nsContentUtils::GetSystemPrincipal(),
      nsILoadInfo::SEC_ALLOW_CROSS_ORIGIN_SEC_CONTEXT_IS_NULL |
          nsILoadInfo::SEC_COOKIES_OMIT,
      nsIContentPolicy::TYPE_OTHER)));
  nsCOMPtr<nsIHttpChannel> http = do_QueryInterface(fetch);
  ASSERT_TRUE(http);
  nsCOMPtr<nsIReferrerInfo> referrer =
      do_CreateInstance("@mozilla.org/referrer-info;1");
  ASSERT_TRUE(referrer);
  ASSERT_TRUE(NS_SUCCEEDED(referrer->Init(
      nsIReferrerInfo::ReferrerPolicyIDL::UNSAFE_URL, true, uri)));
  ASSERT_TRUE(NS_SUCCEEDED(http->SetReferrerInfo(referrer)));

  ASSERT_TRUE(NS_SUCCEEDED(PrepareResolutionFetch(fetch, originLoadInfo)));

  nsCOMPtr<nsIReferrerInfo> actual;
  ASSERT_TRUE(NS_SUCCEEDED(http->GetReferrerInfo(getter_AddRefs(actual))));
  EXPECT_FALSE(actual);

  nsCOMPtr<nsILoadInfo> fetchLoadInfo;
  ASSERT_TRUE(NS_SUCCEEDED(fetch->GetLoadInfo(getter_AddRefs(fetchLoadInfo))));
  EXPECT_EQ(fetchLoadInfo->GetOriginAttributes().mPrivateBrowsingId, 1u);
  EXPECT_EQ(fetchLoadInfo->GetCookiePolicy(), nsILoadInfo::SEC_COOKIES_OMIT);

  nsLoadFlags flags = nsIRequest::LOAD_NORMAL;
  ASSERT_TRUE(NS_SUCCEEDED(fetch->GetLoadFlags(&flags)));
  EXPECT_TRUE(flags & nsIRequest::INHIBIT_PERSISTENT_CACHING);
  EXPECT_TRUE(flags & nsIRequest::LOAD_BYPASS_CACHE);
}
