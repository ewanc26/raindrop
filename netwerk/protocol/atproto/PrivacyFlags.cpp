/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "PrivacyFlags.h"

#include "nsCOMPtr.h"
#include "nsIChannel.h"
#include "nsIHttpChannel.h"
#include "nsILoadInfo.h"
#include "nsIRequest.h"

namespace mozilla::net::atproto {

// Identity lookups must be fresh on every navigation (ADR 0003 change
// detection) and must never be written to the disk cache. Private-browsing
// state is carried by the loadInfo's origin attributes, so it is inherited
// separately from these bits.
uint32_t ResolutionLoadFlags(uint32_t aBaseFlags) {
  return aBaseFlags | nsIRequest::INHIBIT_PERSISTENT_CACHING |
         nsIRequest::LOAD_BYPASS_CACHE;
}

nsresult PrepareResolutionFetch(nsIChannel* aChannel,
                                nsILoadInfo* aOriginLoadInfo) {
  if (aOriginLoadInfo) {
    nsCOMPtr<nsILoadInfo> loadInfo;
    nsresult rv = aChannel->GetLoadInfo(getter_AddRefs(loadInfo));
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(loadInfo, NS_ERROR_UNEXPECTED);
    loadInfo->SetOriginAttributes(aOriginLoadInfo->GetOriginAttributes());
  }

  nsLoadFlags flags = nsIRequest::LOAD_NORMAL;
  nsresult rv = aChannel->GetLoadFlags(&flags);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aChannel->SetLoadFlags(ResolutionLoadFlags(flags));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIHttpChannel> http = do_QueryInterface(aChannel);
  NS_ENSURE_TRUE(http, NS_ERROR_UNEXPECTED);
  return http->SetReferrerInfo(nullptr);
}

}  // namespace mozilla::net::atproto
