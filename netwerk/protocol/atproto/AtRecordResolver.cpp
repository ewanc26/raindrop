/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "AtRecordResolver.h"

#include <algorithm>
#include <vector>

#include "CollectionView.h"
#include "DidResolver.h"
#include "HandleResolver.h"
#include "PdsClient.h"
#include "PrivacyFlags.h"
#include "RecordView.h"
#include "RepoView.h"
#include "mozilla/Logging.h"
#include "mozilla/OriginAttributes.h"
#include "mozilla/Preferences.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsIChannel.h"
#include "nsIDNSByTypeRecord.h"
#include "nsIDNSService.h"
#include "nsIHttpChannel.h"
#include "nsIInputStream.h"
#include "nsILoadInfo.h"
#include "nsIURI.h"
#include "nsNetCID.h"
#include "nsNetUtil.h"
#include "nsStreamUtils.h"
#include "nsStringStream.h"

namespace mozilla {
namespace net {
namespace atproto {

LazyLogModule gAtResolverLog("atproto-resolver");

#define AT_LOG(args) MOZ_LOG(gAtResolverLog, LogLevel::Debug, args)

namespace {

bool EqualsIgnoreCase(std::string_view aLeft, std::string_view aRight) {
  if (aLeft.size() != aRight.size()) {
    return false;
  }
  for (size_t i = 0; i < aLeft.size(); ++i) {
    char left = aLeft[i];
    char right = aRight[i];
    if (left >= 'A' && left <= 'Z') {
      left += 'a' - 'A';
    }
    if (right >= 'A' && right <= 'Z') {
      right += 'a' - 'A';
    }
    if (left != right) {
      return false;
    }
  }
  return true;
}

int32_t EffectivePort(std::string_view aScheme, int32_t aPort) {
  if (aPort != -1) {
    return aPort;
  }
  if (EqualsIgnoreCase(aScheme, "https")) {
    return 443;
  }
  if (EqualsIgnoreCase(aScheme, "http")) {
    return 80;
  }
  return -1;
}

}  // namespace

bool IsSameOrigin(std::string_view aSchemeA, std::string_view aHostA,
                  int32_t aPortA, std::string_view aSchemeB,
                  std::string_view aHostB, int32_t aPortB) {
  return EqualsIgnoreCase(aSchemeA, aSchemeB) &&
         EqualsIgnoreCase(aHostA, aHostB) &&
         EffectivePort(aSchemeA, aPortA) == EffectivePort(aSchemeB, aPortB);
}

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
  return StartHandleDnsLookup();
}

nsresult AtRecordResolver::StartHandleDnsLookup() {
  std::string host;
  HandleError herr = BuildHandleTxtHost(mHandle, host);
  if (herr != HandleError::None) {
    AT_LOG(("atproto: invalid handle authority (%s)\n", HandleErrorName(herr)));
    return NS_ERROR_MALFORMED_URI;
  }

  nsCOMPtr<nsIDNSService> dns = do_GetService(NS_DNSSERVICE_CONTRACTID);
  NS_ENSURE_TRUE(dns, NS_ERROR_UNEXPECTED);

  OriginAttributes oa;
  if (mLoadInfo) {
    oa = mLoadInfo->GetOriginAttributes();
  }

  nsCOMPtr<nsIEventTarget> target = do_GetMainThread();
  NS_ENSURE_TRUE(target, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsICancelable> request;
  nsresult rv = dns->AsyncResolveNative(
      nsCString(host.c_str()), nsIDNSService::RESOLVE_TYPE_TXT,
      nsIDNSService::RESOLVE_DEFAULT_FLAGS, nullptr, this, target, oa,
      getter_AddRefs(request));
  NS_ENSURE_SUCCESS(rv, rv);
  mDnsRequest = request;
  // A cached lookup can call back inside AsyncResolveNative. Do not keep a
  // request that has already completed.
  if (mDnsCompleted) {
    mDnsRequest = nullptr;
  }
  return NS_OK;
}

nsresult AtRecordResolver::StartHandleHttpLookup() {
  std::string url;
  HandleError herr = BuildHandleDidUrl(mHandle, url);
  if (herr != HandleError::None) {
    return NS_ERROR_MALFORMED_URI;
  }
  return StartFetch(url, Phase::Handle);
}

NS_IMETHODIMP
AtRecordResolver::OnLookupComplete(nsICancelable* aRequest,
                                   nsIDNSRecord* aRecord, nsresult aStatus) {
  mDnsCompleted = true;
  mDnsRequest = nullptr;
  if (mCanceled) {
    DeliverFailure(aStatus);
    return NS_OK;
  }

  nsresult rv = NS_OK;
  bool resolved = false;
  if (NS_SUCCEEDED(aStatus) && aRecord) {
    nsCOMPtr<nsIDNSTXTRecord> txt = do_QueryInterface(aRecord);
    if (txt) {
      CopyableTArray<nsCString> records;
      if (NS_SUCCEEDED(txt->GetRecords(records)) && !records.IsEmpty()) {
        std::vector<std::string> strings;
        strings.reserve(records.Length());
        for (const nsCString& record : records) {
          strings.emplace_back(record.BeginReading(), record.Length());
        }
        std::string did;
        HandleError herr = ParseHandleTxtRecords(strings, did);
        if (herr == HandleError::None) {
          mDid = did;
          rv = StartDidDocumentFetch();
          resolved = true;
        } else if (herr == HandleError::TxtMultipleDid) {
          // Ambiguous records must not be silently bypassed: falling back to
          // HTTPS could let whoever controls one channel win.
          SetFailure(FailureKind::HandleResolutionFailed,
                     HandleErrorName(herr));
          DeliverFailure(NS_ERROR_NOT_AVAILABLE);
          return NS_OK;
        }
      }
    }
  }

  if (!resolved) {
    rv = StartHandleHttpLookup();
  }
  if (NS_FAILED(rv)) {
    DeliverFailure(rv);
  }
  return NS_OK;
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
  mRedirectRejected = false;

  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), nsCString(aUrl.c_str()));
  NS_ENSURE_SUCCESS(rv, rv);

  // Remember the expected origin so a cross-origin redirect can be rejected:
  // for DID documents the domain is the identity anchor (ADR 0004), and for
  // handle resolution the handle's own host must answer (ADR 0003). Scheme and
  // port are part of the origin: an https to http downgrade or a port change
  // must fail just as a host change does.
  mExpectedScheme.Truncate(0);
  mExpectedHost.Truncate(0);
  mExpectedPort = -1;
  rv = uri->GetScheme(mExpectedScheme);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = uri->GetHost(mExpectedHost);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = uri->GetPort(&mExpectedPort);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewChannelInternal(getter_AddRefs(channel), uri, mLoadInfo, nullptr,
                             nullptr, this, nsIRequest::LOAD_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  nsLoadFlags flags = nsIRequest::LOAD_NORMAL;
  rv = channel->GetLoadFlags(&flags);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = channel->SetLoadFlags(ResolutionLoadFlags(flags));
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
      mPhase != Phase::Record && mPhase != Phase::Repo &&
      mPhase != Phase::Collection) {
    return NS_OK;
  }

  nsAutoCString chunk;
  nsresult rv = NS_ConsumeStream(aInputStream, aCount, chunk);
  NS_ENSURE_SUCCESS(rv, rv);

  size_t cap = mPhase == Phase::Handle       ? kMaxHandleResponseLength
               : mPhase == Phase::Record     ? kMaxRecordResponseLength
               : mPhase == Phase::Repo       ? kMaxRepoResponseLength
               : mPhase == Phase::Collection ? kMaxCollectionResponseLength
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
  if (mCanceled) {
    mCurrentRequest = nullptr;
    mPhase = Phase::None;
    ForwardStop(aStatus);
    return NS_OK;
  }

  if (mPhase != Phase::Handle && mPhase != Phase::DidDocument &&
      mPhase != Phase::Record && mPhase != Phase::Repo &&
      mPhase != Phase::Collection) {
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
    case Phase::Repo:
      rv = OnRepoStop(aRequest, aStatus);
      break;
    case Phase::Collection:
      rv = OnCollectionStop(aRequest, aStatus);
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
  if (mRedirectRejected) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  if (NS_FAILED(aStatus)) {
    FailureKind kind =
        mPhase == Phase::Handle       ? FailureKind::HandleResolutionFailed
        : mPhase == Phase::Record     ? FailureKind::PdsUnreachable
        : mPhase == Phase::Repo       ? FailureKind::PdsUnreachable
        : mPhase == Phase::Collection ? FailureKind::PdsUnreachable
                                      : FailureKind::DidResolutionFailed;
    SetFailure(kind, "network request failed");
    return aStatus;
  }

  nsCOMPtr<nsIHttpChannel> http = do_QueryInterface(aRequest);
  if (http) {
    uint32_t responseStatus = 0;
    nsresult rv = http->GetResponseStatus(&responseStatus);
    if (NS_SUCCEEDED(rv) && (responseStatus < 200 || responseStatus >= 300)) {
      AT_LOG(("atproto: HTTP status %u\n", responseStatus));
      FailureKind kind = mPhase == Phase::Record && responseStatus == 404
                             ? FailureKind::RecordNotFound
                         : mPhase == Phase::Repo && responseStatus == 404
                             ? FailureKind::RepoNotFound
                         : mPhase == Phase::Collection && responseStatus == 404
                             ? FailureKind::CollectionNotFound
                             : FailureKind::UpstreamRejected;
      SetFailure(kind, "HTTP " + std::to_string(responseStatus));
      return NS_ERROR_NOT_AVAILABLE;
    }
  }

  nsCOMPtr<nsIChannel> responseChannel = do_QueryInterface(aRequest);
  if (responseChannel) {
    nsCOMPtr<nsIURI> finalUri;
    nsresult rv = responseChannel->GetURI(getter_AddRefs(finalUri));
    if (NS_SUCCEEDED(rv) && finalUri) {
      nsAutoCString finalScheme;
      nsAutoCString finalHost;
      int32_t finalPort = -1;
      rv = finalUri->GetScheme(finalScheme);
      if (NS_SUCCEEDED(rv)) {
        rv = finalUri->GetHost(finalHost);
      }
      if (NS_SUCCEEDED(rv)) {
        rv = finalUri->GetPort(&finalPort);
      }
      if (NS_SUCCEEDED(rv) &&
          !IsSameOrigin(mExpectedScheme, mExpectedHost, mExpectedPort,
                        finalScheme, finalHost, finalPort)) {
        AT_LOG(("atproto: response redirected to another origin\n"));
        SetFailure(FailureKind::RedirectRejected, "upstream changed origin");
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
    SetFailure(FailureKind::HandleResolutionFailed, HandleErrorName(herr));
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
      SetFailure(FailureKind::HandleBindingFailed,
                 HandleBindingErrorName(berr));
      return NS_ERROR_NOT_AVAILABLE;
    }
  }

  PdsService service;
  PdsError perr = ExtractPdsEndpoint(
      std::string_view(mBody.BeginReading(), mBody.Length()), mDid, service);
  if (perr != PdsError::None) {
    AT_LOG(("atproto: DID document rejected (%s)\n", PdsErrorName(perr)));
    if (perr == PdsError::OnionEndpoint) {
      mPds = service.mEndpoint;
      SetFailure(FailureKind::OnionEndpointUnsupported, PdsErrorName(perr));
      return NS_ERROR_NOT_AVAILABLE;
    }
    SetFailure(FailureKind::DidDocumentRejected, PdsErrorName(perr));
    return NS_ERROR_FAILURE;
  }

  mPds = service.mEndpoint;

  if (!mUri.mHasCollection) {
    std::string url;
    PdsUrlError uerr = BuildDescribeRepoUrl(service.mEndpoint, mDid, url);
    if (uerr != PdsUrlError::None) {
      AT_LOG(
          ("atproto: describeRepo URL failed (%s)\n", PdsUrlErrorName(uerr)));
      SetFailure(FailureKind::DidDocumentRejected, PdsUrlErrorName(uerr));
      return NS_ERROR_FAILURE;
    }
    return StartFetch(url, Phase::Repo);
  }

  if (!mUri.mHasRecordKey) {
    std::string url;
    PdsUrlError uerr = BuildListRecordsUrl(service.mEndpoint, mDid,
                                           mUri.mCollection, mUri.mCursor, url);
    if (uerr != PdsUrlError::None) {
      AT_LOG(("atproto: listRecords URL failed (%s)\n", PdsUrlErrorName(uerr)));
      SetFailure(FailureKind::DidDocumentRejected, PdsUrlErrorName(uerr));
      return NS_ERROR_FAILURE;
    }
    return StartFetch(url, Phase::Collection);
  }

  std::string url;
  PdsUrlError uerr = BuildGetRecordUrl(service.mEndpoint, mDid,
                                       mUri.mCollection, mUri.mRecordKey, url);
  if (uerr != PdsUrlError::None) {
    AT_LOG(("atproto: getRecord URL failed (%s)\n", PdsUrlErrorName(uerr)));
    SetFailure(FailureKind::DidDocumentRejected, PdsUrlErrorName(uerr));
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
    SetFailure(FailureKind::RecordInvalid, RecordViewErrorName(verr));
    return NS_ERROR_NOT_AVAILABLE;
  }

  RecordViewProvenance provenance{mDid, mHandle, mPds, mUri.mCollection,
                                  mUri.mRecordKey};
  RecordViewOptions options{
      Preferences::GetBool("network.atprotocol.record.showProvenance", true),
      Preferences::GetBool("network.atprotocol.record.showRawJson", true),
      Preferences::GetBool("network.atprotocol.record.showImages", true)};
  return DeliverHtml(RenderRecordHtml(data, provenance, options));
}

nsresult AtRecordResolver::OnRepoStop(nsIRequest* aRequest, nsresult aStatus) {
  nsresult rv = CheckResponse(aRequest, aStatus);
  NS_ENSURE_SUCCESS(rv, rv);

  RepoViewData data;
  RepoViewError verr = ParseRepoResponse(
      std::string_view(mBody.BeginReading(), mBody.Length()), data);
  if (verr != RepoViewError::None) {
    AT_LOG(("atproto: repo view failed (%s)\n", RepoViewErrorName(verr)));
    SetFailure(FailureKind::RepoInvalid, RepoViewErrorName(verr));
    return NS_ERROR_NOT_AVAILABLE;
  }

  return DeliverHtml(
      RenderRepoHtml(data, RepoViewProvenance{mDid, mHandle, mPds}));
}

nsresult AtRecordResolver::OnCollectionStop(nsIRequest* aRequest,
                                            nsresult aStatus) {
  nsresult rv = CheckResponse(aRequest, aStatus);
  NS_ENSURE_SUCCESS(rv, rv);

  CollectionViewData data;
  CollectionViewError verr = ParseCollectionResponse(
      std::string_view(mBody.BeginReading(), mBody.Length()), data);
  if (verr != CollectionViewError::None) {
    AT_LOG(("atproto: collection view failed (%s)\n",
            CollectionViewErrorName(verr)));
    SetFailure(FailureKind::CollectionInvalid, CollectionViewErrorName(verr));
    return NS_ERROR_NOT_AVAILABLE;
  }

  return DeliverHtml(RenderCollectionHtml(
      data, CollectionViewProvenance{mDid, mHandle, mPds, mUri.mCollection,
                                     mUri.mCursor}));
}

nsresult AtRecordResolver::DeliverHtml(std::string_view aHtml) {
  nsCOMPtr<nsIInputStream> stream;
  nsresult rv = NS_NewCStringInputStream(getter_AddRefs(stream),
                                         nsCString(aHtml.data(), aHtml.size()));
  NS_ENSURE_SUCCESS(rv, rv);

  ForwardStart();
  if (mTarget && !mDeliveredStop) {
    rv = mTarget->OnDataAvailable(mOuterRequest, stream, 0, aHtml.size());
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

void AtRecordResolver::SetFailure(FailureKind aKind, std::string_view aDetail) {
  mFailure.mKind = aKind;
  mFailure.mAuthority = mUri.mAuthority;
  mFailure.mDid = mDid;
  mFailure.mPds = mPds;
  mFailure.mDetail.assign(aDetail.data(), aDetail.size());
}

void AtRecordResolver::DeliverFailure(nsresult aStatus) {
  mPhase = Phase::None;
  if (mCanceled) {
    ForwardStop(aStatus);
    return;
  }

  if (mFailure.mAuthority.empty()) {
    mFailure.mAuthority = mUri.mAuthority;
    mFailure.mDid = mDid;
    mFailure.mPds = mPds;
  }
  DeliverHtml(RenderFailureHtml(mFailure));
  // Deliver the explanation as content rather than a bare status code so the
  // browser shows an honest, step-specific page instead of a generic error.
  ForwardStop(NS_OK);
}

NS_IMETHODIMP
AtRecordResolver::Cancel(nsresult aReason) {
  mCanceled = true;
  if (mDnsRequest) {
    mDnsRequest->Cancel(aReason);
  }
  if (mCurrentRequest) {
    mCurrentRequest->Cancel(aReason);
  }
  return NS_OK;
}

NS_IMETHODIMP
AtRecordResolver::GetInterface(const nsIID& aIID, void** aResult) {
  if (aIID.Equals(NS_GET_IID(nsIChannelEventSink))) {
    nsCOMPtr<nsIChannelEventSink> self(this);
    self.forget(aResult);
    return NS_OK;
  }
  return NS_ERROR_NO_INTERFACE;
}

NS_IMETHODIMP
AtRecordResolver::AsyncOnChannelRedirect(
    nsIChannel* aOldChannel, nsIChannel* aNewChannel, uint32_t aFlags,
    nsIAsyncVerifyRedirectCallback* aCallback) {
  nsCOMPtr<nsIURI> newUri;
  nsresult rv = aNewChannel->GetURI(getter_AddRefs(newUri));
  if (NS_SUCCEEDED(rv)) {
    nsAutoCString scheme;
    nsAutoCString host;
    int32_t port = -1;
    rv = newUri->GetScheme(scheme);
    if (NS_SUCCEEDED(rv)) {
      rv = newUri->GetHost(host);
    }
    if (NS_SUCCEEDED(rv)) {
      rv = newUri->GetPort(&port);
    }
    if (NS_SUCCEEDED(rv) && !IsSameOrigin(mExpectedScheme, mExpectedHost,
                                          mExpectedPort, scheme, host, port)) {
      AT_LOG(("atproto: rejected cross-origin redirect hop\n"));
      mRedirectRejected = true;
      SetFailure(FailureKind::RedirectRejected, "upstream changed origin");
      rv = NS_ERROR_NOT_AVAILABLE;
    }
  }
  aCallback->OnRedirectVerifyCallback(rv);
  return NS_OK;
}

NS_IMPL_ISUPPORTS(AtRecordResolver, nsIStreamListener, nsIRequestObserver,
                  nsIDNSListener, nsICancelable, nsIInterfaceRequestor,
                  nsIChannelEventSink)

}  // namespace atproto
}  // namespace net
}  // namespace mozilla
