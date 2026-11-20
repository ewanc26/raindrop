/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "AtRecordResolver.h"

#include <algorithm>

#include "DidResolver.h"
#include "HandleResolver.h"
#include "PdsClient.h"
#include "RecordView.h"
#include "mozilla/Logging.h"
#include "nsIChannel.h"
#include "nsIHttpChannel.h"
#include "nsIInputStream.h"
#include "nsILoadInfo.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsStreamUtils.h"
#include "nsStringStream.h"

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

  nsresult rv = StartResolution();
  if (NS_FAILED(rv)) {
    return Err(rv);
  }

  nsCOMPtr<nsICancelable> cancelable(this);
  return RequestOrCancelable(WrapNotNull(cancelable));
}

nsresult AtRecordResolver::StartResolution() {
  if (mUri.mAuthority.compare(0, 4, "did:") == 0) {
    Did did;
    DidError derr = ParseDid(mUri.mAuthority, did);
    if (derr != DidError::None) {
      AT_LOG(("atproto: invalid DID authority (%s)\n", DidErrorName(derr)));
      return NS_ERROR_MALFORMED_URI;
    }
    mDid = did.mFull;
    return StartDidDocumentFetch();
  }

  mHandle = mUri.mAuthority;
  std::string url;
  HandleError herr = BuildHandleDidUrl(mHandle, url);
  if (herr != HandleError::None) {
    AT_LOG(("atproto: invalid handle authority (%s)\n", HandleErrorName(herr)));
    return NS_ERROR_MALFORMED_URI;
  }
  return StartFetch(url, Phase::Handle);
}

nsresult AtRecordResolver::StartDidDocumentFetch() {
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

  // Remember the expected host so a cross-origin redirect can be rejected:
  // for DID documents the domain is the identity anchor (ADR 0004), and for
  // handle resolution the handle's own host must answer (ADR 0003).
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
  return NS_OK;
}

NS_IMETHODIMP
AtRecordResolver::OnDataAvailable(nsIRequest* aRequest,
                                  nsIInputStream* aInputStream,
                                  uint64_t aSourceOffset, uint32_t aCount) {
  if (mCanceled) {
    return NS_BINDING_ABORTED;
  }

  if (mPhase != Phase::Handle && mPhase != Phase::DidDocument &&
      mPhase != Phase::Record) {
    return NS_OK;
  }

  nsAutoCString chunk;
  nsresult rv = NS_ConsumeStream(aInputStream, aCount, chunk);
  NS_ENSURE_SUCCESS(rv, rv);

  size_t cap = mPhase == Phase::Handle    ? kMaxHandleResponseLength
               : mPhase == Phase::Record ? kMaxRecordResponseLength
                                         : kMaxDidDocumentLength;
  // Keep one byte past the cap: the parsers reject bodies larger than the cap,
  // so truncating exactly at the cap would turn an oversized response into a
  // valid-looking one.
  size_t limit = cap + 1;
  if (mBody.Length() < limit) {
    uint32_t remaining = static_cast<uint32_t>(limit - mBody.Length());
    mBody.Append(
        Substring(chunk, 0, std::min<uint32_t>(chunk.Length(), remaining)));
  }
  return NS_OK;
}

NS_IMETHODIMP
AtRecordResolver::OnStopRequest(nsIRequest* aRequest, nsresult aStatus) {
  if (mPhase != Phase::Handle && mPhase != Phase::DidDocument &&
      mPhase != Phase::Record) {
    return NS_OK;
  }

  Phase finished = mPhase;
  // Clear the finished request before the handler may install the next fetch
  // as mCurrentRequest, so cancellation always reaches the in-flight request.
  mCurrentRequest = nullptr;
  nsresult rv;
  switch (finished) {
    case Phase::Handle:
      rv = OnHandleStop(aRequest, aStatus);
      break;
    case Phase::DidDocument:
      rv = OnDidDocumentStop(aRequest, aStatus);
      break;
    case Phase::Record:
      rv = OnRecordStop(aRequest, aStatus);
      break;
    default:
      return NS_OK;
  }
  if (NS_FAILED(rv)) {
    DeliverFailure(rv);
  }
  return NS_OK;
}

nsresult AtRecordResolver::CheckResponse(nsIRequest* aRequest,
                                         nsresult aStatus) {
  NS_ENSURE_SUCCESS(aStatus, aStatus);

  nsCOMPtr<nsIHttpChannel> http = do_QueryInterface(aRequest);
  if (http) {
    uint32_t responseStatus = 0;
    nsresult rv = http->GetResponseStatus(&responseStatus);
    if (NS_SUCCEEDED(rv) && (responseStatus < 200 || responseStatus >= 300)) {
      AT_LOG(("atproto: HTTP status %u\n", responseStatus));
      return NS_ERROR_NOT_AVAILABLE;
    }
  }

  nsCOMPtr<nsIChannel> responseChannel = do_QueryInterface(aRequest);
  if (responseChannel) {
    nsCOMPtr<nsIURI> finalUri;
    nsresult rv = responseChannel->GetURI(getter_AddRefs(finalUri));
    if (NS_SUCCEEDED(rv) && finalUri) {
      nsAutoCString finalHost;
      rv = finalUri->GetHost(finalHost);
      if (NS_SUCCEEDED(rv) && !finalHost.Equals(mExpectedHost)) {
        AT_LOG(("atproto: response redirected to another host\n"));
        return NS_ERROR_NOT_AVAILABLE;
      }
    }
  }
  return NS_OK;
}

nsresult AtRecordResolver::OnHandleStop(nsIRequest* aRequest,
                                        nsresult aStatus) {
  nsresult rv = CheckResponse(aRequest, aStatus);
  NS_ENSURE_SUCCESS(rv, rv);

  HandleError herr = ParseHandleDidResponse(
      std::string_view(mBody.BeginReading(), mBody.Length()), mDid);
  if (herr != HandleError::None) {
    AT_LOG(("atproto: handle resolution failed (%s)\n", HandleErrorName(herr)));
    return NS_ERROR_NOT_AVAILABLE;
  }
  return StartDidDocumentFetch();
}

nsresult AtRecordResolver::OnDidDocumentStop(nsIRequest* aRequest,
                                             nsresult aStatus) {
  nsresult rv = CheckResponse(aRequest, aStatus);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mHandle.empty()) {
    HandleBindingError berr = VerifyHandleBinding(
        std::string_view(mBody.BeginReading(), mBody.Length()), mHandle);
    if (berr != HandleBindingError::None) {
      AT_LOG(("atproto: handle reverse binding failed (%s)\n",
              HandleBindingErrorName(berr)));
      return NS_ERROR_NOT_AVAILABLE;
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

  mPds = service.mEndpoint;

  std::string url;
  PdsUrlError uerr = BuildGetRecordUrl(service.mEndpoint, mDid, mUri.mCollection,
                                       mUri.mRecordKey, url);
  if (uerr != PdsUrlError::None) {
    AT_LOG(("atproto: getRecord URL failed (%s)\n", PdsUrlErrorName(uerr)));
    return NS_ERROR_FAILURE;
  }

  return StartFetch(url, Phase::Record);
}

nsresult AtRecordResolver::OnRecordStop(nsIRequest* aRequest,
                                        nsresult aStatus) {
  nsresult rv = CheckResponse(aRequest, aStatus);
  NS_ENSURE_SUCCESS(rv, rv);

  RecordViewData data;
  RecordViewError verr = ParseRecordResponse(
      std::string_view(mBody.BeginReading(), mBody.Length()), data);
  if (verr != RecordViewError::None) {
    AT_LOG(("atproto: record view failed (%s)\n", RecordViewErrorName(verr)));
    return NS_ERROR_NOT_AVAILABLE;
  }

  RecordViewProvenance provenance{mDid, mHandle, mPds, mUri.mCollection,
                                  mUri.mRecordKey};
  std::string html = RenderRecordHtml(data, provenance);

  nsCOMPtr<nsIInputStream> stream;
  rv = NS_NewCStringInputStream(getter_AddRefs(stream),
                               nsCString(html.data(), html.size()));
  NS_ENSURE_SUCCESS(rv, rv);

  ForwardStart();
  if (mTarget && !mDeliveredStop) {
    rv = mTarget->OnDataAvailable(mOuterRequest, stream, 0, html.size());
  }
  ForwardStop(NS_OK);
  return rv;
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
