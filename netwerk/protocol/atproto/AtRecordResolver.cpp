/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "AtRecordResolver.h"

#include <algorithm>

#include "DidResolver.h"
#include "PdsClient.h"
#include "mozilla/Logging.h"
#include "nsIChannel.h"
#include "nsIHttpChannel.h"
#include "nsIInputStream.h"
#include "nsILoadInfo.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsStreamUtils.h"

namespace mozilla {
namespace net {
namespace atproto {

LazyLogModule gAtResolverLog("atproto-resolver");

#define AT_LOG(args) MOZ_LOG(gAtResolverLog, LogLevel::Debug, args)

AtRecordResolver::AtRecordResolver(const AtURI& aUri, nsILoadInfo* aLoadInfo)
    : mUri(aUri), mLoadInfo(aLoadInfo) {}

RequestOrReason AtRecordResolver::Start(nsIStreamListener* aTarget) {
  mTarget = aTarget;
  mOuterRequest = do_QueryInterface(aTarget);
  if (!mOuterRequest) {
    return Err(NS_ERROR_UNEXPECTED);
  }

  nsresult rv = StartDidDocumentFetch();
  if (NS_FAILED(rv)) {
    return Err(rv);
  }

  nsCOMPtr<nsICancelable> cancelable(this);
  return RequestOrCancelable(WrapNotNull(cancelable));
}

nsresult AtRecordResolver::StartDidDocumentFetch() {
  Did did;
  DidError derr = ParseDid(mUri.mAuthority, did);
  if (derr != DidError::None) {
    AT_LOG(("atproto: invalid DID authority (%s)\n", DidErrorName(derr)));
    return NS_ERROR_MALFORMED_URI;
  }
  mDid = did.mFull;

  std::string url;
  DidUrlError uerr = BuildDidDocumentUrl(mDid, url);
  if (uerr != DidUrlError::None) {
    AT_LOG(("atproto: DID document URL failed (%s)\n", DidUrlErrorName(uerr)));
    return NS_ERROR_MALFORMED_URI;
  }
  return StartFetch(url, Phase::DidDocument);
}

nsresult AtRecordResolver::StartFetch(const std::string& aUrl, Phase aPhase) {
  mPhase = aPhase;
  mBody.Truncate(0);

  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), nsCString(aUrl.c_str()));
  NS_ENSURE_SUCCESS(rv, rv);

  // Remember the expected host so a cross-origin redirect of the DID document
  // can be rejected: the DID's domain is the identity anchor (ADR 0004).
  mExpectedHost.Truncate(0);
  rv = uri->GetHost(mExpectedHost);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewChannelInternal(getter_AddRefs(channel), uri, mLoadInfo, nullptr,
                             nullptr, nullptr, nsIRequest::LOAD_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIHttpChannel> http = do_QueryInterface(channel);
  NS_ENSURE_TRUE(http, NS_ERROR_UNEXPECTED);
  rv = http->SetRequestMethod("GET"_ns);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = http->SetRequestHeader("Accept"_ns, "application/json"_ns, false);
  NS_ENSURE_SUCCESS(rv, rv);

  mCurrentRequest = channel;
  rv = channel->AsyncOpen(this);
  if (NS_FAILED(rv)) {
    mCurrentRequest = nullptr;
    mPhase = Phase::None;
    return rv;
  }
  return NS_OK;
}

NS_IMETHODIMP
AtRecordResolver::OnStartRequest(nsIRequest* aRequest) {
  mCurrentRequest = aRequest;
  if (mPhase == Phase::Record) {
    ForwardStart();
  }
  return NS_OK;
}

NS_IMETHODIMP
AtRecordResolver::OnDataAvailable(nsIRequest* aRequest,
                                  nsIInputStream* aInputStream,
                                  uint64_t aSourceOffset, uint32_t aCount) {
  if (mCanceled) {
    return NS_BINDING_ABORTED;
  }

  if (mPhase == Phase::Record) {
    if (mTarget && !mDeliveredStop) {
      return mTarget->OnDataAvailable(mOuterRequest, aInputStream,
                                      aSourceOffset, aCount);
    }
    return NS_OK;
  }

  if (mPhase != Phase::DidDocument) {
    return NS_OK;
  }

  nsAutoCString chunk;
  nsresult rv = NS_ConsumeStream(aInputStream, aCount, chunk);
  NS_ENSURE_SUCCESS(rv, rv);

  // Stop accumulating past the cap. ExtractPdsEndpoint rejects the oversized
  // document in OnStopRequest, so truncation cannot smuggle in a valid doc.
  if (mBody.Length() < kMaxDidDocumentLength) {
    uint32_t remaining =
        static_cast<uint32_t>(kMaxDidDocumentLength - mBody.Length());
    mBody.Append(
        Substring(chunk, 0, std::min<uint32_t>(chunk.Length(), remaining)));
  }
  return NS_OK;
}

NS_IMETHODIMP
AtRecordResolver::OnStopRequest(nsIRequest* aRequest, nsresult aStatus) {
  if (mPhase == Phase::Record) {
    ForwardStop(aStatus);
    mCurrentRequest = nullptr;
    return NS_OK;
  }

  if (mPhase != Phase::DidDocument) {
    return NS_OK;
  }

  // Clear the finished DID-document request before OnDidDocumentStop may
  // install the record fetch as the new mCurrentRequest.
  mCurrentRequest = nullptr;
  nsresult rv = OnDidDocumentStop(aRequest, aStatus);
  if (NS_FAILED(rv)) {
    DeliverFailure(rv);
  }
  return NS_OK;
}

nsresult AtRecordResolver::OnDidDocumentStop(nsIRequest* aRequest,
                                             nsresult aStatus) {
  NS_ENSURE_SUCCESS(aStatus, aStatus);

  nsCOMPtr<nsIHttpChannel> http = do_QueryInterface(aRequest);
  if (http) {
    uint32_t responseStatus = 0;
    nsresult rv = http->GetResponseStatus(&responseStatus);
    if (NS_SUCCEEDED(rv) && (responseStatus < 200 || responseStatus >= 300)) {
      AT_LOG(("atproto: DID document HTTP status %u\n", responseStatus));
      return NS_ERROR_NOT_AVAILABLE;
    }
  }

  // A cross-origin redirect moves the DID document off the domain named in
  // the DID, breaking the identity anchor. Reject it (ADR 0004).
  nsCOMPtr<nsIChannel> docChannel = do_QueryInterface(aRequest);
  if (docChannel) {
    nsCOMPtr<nsIURI> finalUri;
    nsresult rv = docChannel->GetURI(getter_AddRefs(finalUri));
    if (NS_SUCCEEDED(rv) && finalUri) {
      nsAutoCString finalHost;
      rv = finalUri->GetHost(finalHost);
      if (NS_SUCCEEDED(rv) && !finalHost.Equals(mExpectedHost)) {
        AT_LOG(("atproto: DID document redirected to another host\n"));
        return NS_ERROR_NOT_AVAILABLE;
      }
    }
  }

  PdsService service;
  PdsError perr = ExtractPdsEndpoint(
      std::string_view(mBody.BeginReading(), mBody.Length()), mDid, service);
  if (perr != PdsError::None) {
    AT_LOG(("atproto: DID document rejected (%s)\n", PdsErrorName(perr)));
    return perr == PdsError::OnionEndpoint ? NS_ERROR_NOT_AVAILABLE
                                           : NS_ERROR_FAILURE;
  }

  std::string url;
  PdsUrlError uerr = BuildGetRecordUrl(service.mEndpoint, mDid, mUri.mCollection,
                                       mUri.mRecordKey, url);
  if (uerr != PdsUrlError::None) {
    AT_LOG(("atproto: getRecord URL failed (%s)\n", PdsUrlErrorName(uerr)));
    return NS_ERROR_FAILURE;
  }

  return StartFetch(url, Phase::Record);
}

void AtRecordResolver::ForwardStart() {
  if (mDeliveredStart || !mTarget) {
    return;
  }
  mDeliveredStart = true;
  mTarget->OnStartRequest(mOuterRequest);
}

void AtRecordResolver::ForwardStop(nsresult aStatus) {
  ForwardStart();
  if (mDeliveredStop || !mTarget) {
    return;
  }
  mDeliveredStop = true;
  mTarget->OnStopRequest(mOuterRequest, aStatus);
}

void AtRecordResolver::DeliverFailure(nsresult aStatus) {
  mPhase = Phase::None;
  ForwardStop(aStatus);
}

NS_IMETHODIMP
AtRecordResolver::Cancel(nsresult aReason) {
  mCanceled = true;
  if (mCurrentRequest) {
    mCurrentRequest->Cancel(aReason);
  }
  return NS_OK;
}

NS_IMPL_ISUPPORTS(AtRecordResolver, nsIStreamListener, nsIRequestObserver,
                  nsICancelable)

}  // namespace atproto
}  // namespace net
}  // namespace mozilla
