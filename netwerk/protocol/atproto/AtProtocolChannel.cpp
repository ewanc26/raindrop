/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "AtProtocolChannel.h"

#include <memory>
#include <utility>

#include "AtRecordResolver.h"
#include "NullHttpChannel.h"
#include "mozilla/RefPtr.h"
#include "mozilla/UniquePtr.h"
#include "nsIChannel.h"
#include "nsIHttpChannel.h"
#include "nsIHttpHeaderVisitor.h"
#include "nsILoadInfo.h"
#include "nsIReferrerInfo.h"
#include "nsIURI.h"
#include "nsTArray.h"

// Mirrors NS_FORWARD_NSIHTTPCHANNEL, except SetSource moves the unique_ptr
// because NullHttpChannel's generated setter accepts it by value.
#define AT_FORWARD_NSIHTTPCHANNEL(_to) \
  [[nodiscard]] NS_IMETHOD GetRequestMethod(nsACString& aRequestMethod) override { return _to GetRequestMethod(aRequestMethod); } \
  [[nodiscard]] NS_IMETHOD SetRequestMethod(const nsACString& aRequestMethod) override { return _to SetRequestMethod(aRequestMethod); } \
  using nsIHttpChannel::GetReferrerInfo; \
  [[nodiscard]] NS_IMETHOD GetReferrerInfo(nsIReferrerInfo **aReferrerInfo) override { return _to GetReferrerInfo(aReferrerInfo); } \
  [[nodiscard]] NS_IMETHOD SetReferrerInfo(nsIReferrerInfo *aReferrerInfo) override { return _to SetReferrerInfo(aReferrerInfo); } \
  [[nodiscard]] NS_IMETHOD SetReferrerInfoWithoutClone(nsIReferrerInfo *aReferrerInfo) override { return _to SetReferrerInfoWithoutClone(aReferrerInfo); } \
  [[nodiscard]] NS_IMETHOD GetProtocolVersion(nsACString& aProtocolVersion) override { return _to GetProtocolVersion(aProtocolVersion); } \
  [[nodiscard]] NS_IMETHOD GetTransferSize(uint64_t *aTransferSize) override { return _to GetTransferSize(aTransferSize); } \
  [[nodiscard]] NS_IMETHOD GetRequestSize(uint64_t *aRequestSize) override { return _to GetRequestSize(aRequestSize); } \
  [[nodiscard]] NS_IMETHOD GetDecodedBodySize(uint64_t *aDecodedBodySize) override { return _to GetDecodedBodySize(aDecodedBodySize); } \
  [[nodiscard]] NS_IMETHOD GetEncodedBodySize(uint64_t *aEncodedBodySize) override { return _to GetEncodedBodySize(aEncodedBodySize); } \
  [[nodiscard]] NS_IMETHOD GetRequestHeader(const nsACString& aHeader, nsACString& _retval) override { return _to GetRequestHeader(aHeader, _retval); } \
  [[nodiscard]] NS_IMETHOD SetRequestHeader(const nsACString& aHeader, const nsACString& aValue, bool aMerge) override { return _to SetRequestHeader(aHeader, aValue, aMerge); } \
  [[nodiscard]] NS_IMETHOD SetNewReferrerInfo(const nsACString& aUrl, nsIReferrerInfo::ReferrerPolicyIDL aPolicy, bool aSendReferrer) override { return _to SetNewReferrerInfo(aUrl, aPolicy, aSendReferrer); } \
  [[nodiscard]] NS_IMETHOD SetEmptyRequestHeader(const nsACString& aHeader) override { return _to SetEmptyRequestHeader(aHeader); } \
  [[nodiscard]] NS_IMETHOD VisitRequestHeaders(nsIHttpHeaderVisitor *aVisitor) override { return _to VisitRequestHeaders(aVisitor); } \
  [[nodiscard]] NS_IMETHOD VisitNonDefaultRequestHeaders(nsIHttpHeaderVisitor *aVisitor) override { return _to VisitNonDefaultRequestHeaders(aVisitor); } \
  [[nodiscard]] NS_IMETHOD ShouldStripRequestBodyHeader(const nsACString& aMethod, bool *_retval) override { return _to ShouldStripRequestBodyHeader(aMethod, _retval); } \
  [[nodiscard]] NS_IMETHOD GetAllowSTS(bool *aAllowSTS) override { return _to GetAllowSTS(aAllowSTS); } \
  [[nodiscard]] NS_IMETHOD SetAllowSTS(bool aAllowSTS) override { return _to SetAllowSTS(aAllowSTS); } \
  [[nodiscard]] NS_IMETHOD GetRedirectionLimit(uint32_t *aRedirectionLimit) override { return _to GetRedirectionLimit(aRedirectionLimit); } \
  [[nodiscard]] NS_IMETHOD SetRedirectionLimit(uint32_t aRedirectionLimit) override { return _to SetRedirectionLimit(aRedirectionLimit); } \
  [[nodiscard]] NS_IMETHOD GetResponseStatus(uint32_t *aResponseStatus) override { return _to GetResponseStatus(aResponseStatus); } \
  [[nodiscard]] NS_IMETHOD GetResponseStatusText(nsACString& aResponseStatusText) override { return _to GetResponseStatusText(aResponseStatusText); } \
  [[nodiscard]] NS_IMETHOD GetRequestSucceeded(bool *aRequestSucceeded) override { return _to GetRequestSucceeded(aRequestSucceeded); } \
  [[nodiscard]] NS_IMETHOD GetIsMainDocumentChannel(bool *aIsMainDocumentChannel) override { return _to GetIsMainDocumentChannel(aIsMainDocumentChannel); } \
  [[nodiscard]] NS_IMETHOD SetIsMainDocumentChannel(bool aIsMainDocumentChannel) override { return _to SetIsMainDocumentChannel(aIsMainDocumentChannel); } \
  [[nodiscard]] NS_IMETHOD GetResponseHeader(const nsACString& header, nsACString& _retval) override { return _to GetResponseHeader(header, _retval); } \
  [[nodiscard]] NS_IMETHOD SetResponseHeader(const nsACString& header, const nsACString& value, bool merge) override { return _to SetResponseHeader(header, value, merge); } \
  [[nodiscard]] NS_IMETHOD VisitResponseHeaders(nsIHttpHeaderVisitor *aVisitor) override { return _to VisitResponseHeaders(aVisitor); } \
  [[nodiscard]] NS_IMETHOD GetOriginalResponseHeader(const nsACString& aHeader, nsIHttpHeaderVisitor *aVisitor) override { return _to GetOriginalResponseHeader(aHeader, aVisitor); } \
  [[nodiscard]] NS_IMETHOD VisitOriginalResponseHeaders(nsIHttpHeaderVisitor *aVisitor) override { return _to VisitOriginalResponseHeaders(aVisitor); } \
  [[nodiscard]] NS_IMETHOD IsNoStoreResponse(bool *_retval) override { return _to IsNoStoreResponse(_retval); } \
  [[nodiscard]] NS_IMETHOD IsNoCacheResponse(bool *_retval) override { return _to IsNoCacheResponse(_retval); } \
  [[nodiscard]] NS_IMETHOD RedirectTo(nsIURI *aTargetURI) override { return _to RedirectTo(aTargetURI); } \
  [[nodiscard]] NS_IMETHOD UpgradeToSecure(void) override { return _to UpgradeToSecure(); } \
  [[nodiscard]] NS_IMETHOD GetRequestContextID(uint64_t *aRequestContextID) override { return _to GetRequestContextID(aRequestContextID); } \
  [[nodiscard]] NS_IMETHOD SetRequestContextID(uint64_t aRequestContextID) override { return _to SetRequestContextID(aRequestContextID); } \
  [[nodiscard]] NS_IMETHOD GetTopLevelContentWindowId(uint64_t *aTopLevelContentWindowId) override { return _to GetTopLevelContentWindowId(aTopLevelContentWindowId); } \
  [[nodiscard]] NS_IMETHOD SetTopLevelContentWindowId(uint64_t aTopLevelContentWindowId) override { return _to SetTopLevelContentWindowId(aTopLevelContentWindowId); } \
  [[nodiscard]] NS_IMETHOD GetBrowserId(uint64_t *aBrowserId) override { return _to GetBrowserId(aBrowserId); } \
  [[nodiscard]] NS_IMETHOD SetBrowserId(uint64_t aBrowserId) override { return _to SetBrowserId(aBrowserId); } \
  NS_IMETHOD LogBlockedCORSRequest(const nsAString& aMessage, const nsACString& aCategory, bool aIsWarning) override { return _to LogBlockedCORSRequest(aMessage, aCategory, aIsWarning); } \
  NS_IMETHOD LogMimeTypeMismatch(const nsACString& aMessageName, bool aWarning, const nsAString& aURL, const nsAString& aContentType) override { return _to LogMimeTypeMismatch(aMessageName, aWarning, aURL, aContentType); } \
  virtual void SetSource(mozilla::UniquePtr<mozilla::ProfileChunkedBuffer> aSource) override { return _to SetSource(std::move(aSource)); } \
  [[nodiscard]] NS_IMETHOD GetClassicScriptHintCharset(nsAString& aClassicScriptHintCharset) override { return _to GetClassicScriptHintCharset(aClassicScriptHintCharset); } \
  [[nodiscard]] NS_IMETHOD SetClassicScriptHintCharset(const nsAString& aClassicScriptHintCharset) override { return _to SetClassicScriptHintCharset(aClassicScriptHintCharset); } \
  [[nodiscard]] NS_IMETHOD GetDocumentCharacterSet(nsAString& aDocumentCharacterSet) override { return _to GetDocumentCharacterSet(aDocumentCharacterSet); } \
  [[nodiscard]] NS_IMETHOD SetDocumentCharacterSet(const nsAString& aDocumentCharacterSet) override { return _to SetDocumentCharacterSet(aDocumentCharacterSet); } \
  [[nodiscard]] NS_IMETHOD GetRequestObserversCalled(bool *aRequestObserversCalled) override { return _to GetRequestObserversCalled(aRequestObserversCalled); } \
  [[nodiscard]] NS_IMETHOD SetRequestObserversCalled(bool aRequestObserversCalled) override { return _to SetRequestObserversCalled(aRequestObserversCalled); } \
  [[nodiscard]] NS_IMETHOD GetIsUserAgentHeaderOutdated(bool *aIsUserAgentHeaderOutdated) override { return _to GetIsUserAgentHeaderOutdated(aIsUserAgentHeaderOutdated); } \
  [[nodiscard]] NS_IMETHOD SetIsUserAgentHeaderOutdated(bool aIsUserAgentHeaderOutdated) override { return _to SetIsUserAgentHeaderOutdated(aIsUserAgentHeaderOutdated); } \
  NS_IMETHOD GetDecompressDictionary(mozilla::net::DictionaryCacheEntry * * aDecompressDictionary) override { return _to GetDecompressDictionary(aDecompressDictionary); } \
  NS_IMETHOD SetDecompressDictionary(mozilla::net::DictionaryCacheEntry * aDecompressDictionary) override { return _to SetDecompressDictionary(aDecompressDictionary); } 

namespace mozilla::net::atproto {

namespace {

nsCString CopyString(const nsACString& aString) {
  nsCString result;
  result.Assign(aString);
  return result;
}

class AtDocumentChannelBase : public nsIHttpChannel {
 public:
  NS_DECL_ISUPPORTS
  NS_FORWARD_NSIREQUEST(mInner->)
  NS_FORWARD_NSICHANNEL(mInner->)
  AT_FORWARD_NSIHTTPCHANNEL(mNull.get()->)

 protected:
  explicit AtDocumentChannelBase(nsIChannel* aInner)
      : mInner(aInner), mNull(new NullHttpChannel()) {}
  virtual ~AtDocumentChannelBase() = default;

  nsCOMPtr<nsIChannel> mInner;
  RefPtr<NullHttpChannel> mNull;
};

class AtDocumentChannel final : public AtDocumentChannelBase {
 public:
  explicit AtDocumentChannel(nsIChannel* aInner)
      : AtDocumentChannelBase(aInner) {}

  NS_IMETHOD GetChannelId(uint64_t* aId) override;
  NS_IMETHOD SetChannelId(uint64_t aId) override;
  NS_IMETHOD GetRequestMethod(nsACString& aMethod) override;
  NS_IMETHOD GetReferrerInfo(nsIReferrerInfo** aReferrerInfo) override;
  NS_IMETHOD SetReferrerInfo(nsIReferrerInfo* aReferrerInfo) override;
  NS_IMETHOD GetTransferSize(uint64_t* aSize) override;
  NS_IMETHOD GetRequestSize(uint64_t* aSize) override;
  NS_IMETHOD GetDecodedBodySize(uint64_t* aSize) override;
  NS_IMETHOD GetEncodedBodySize(uint64_t* aSize) override;
  NS_IMETHOD GetAllowSTS(bool* aAllowSTS) override;
  NS_IMETHOD GetResponseStatus(uint32_t* aStatus) override;
  NS_IMETHOD GetResponseStatusText(nsACString& aStatusText) override;
  NS_IMETHOD GetRequestSucceeded(bool* aSucceeded) override;
  NS_IMETHOD GetResponseHeader(const nsACString& aHeader,
                               nsACString& aValue) override;
  NS_IMETHOD SetResponseHeader(const nsACString& aHeader,
                               const nsACString& aValue, bool aMerge) override;
  NS_IMETHOD VisitResponseHeaders(nsIHttpHeaderVisitor* aVisitor) override;
  NS_IMETHOD GetOriginalResponseHeader(
      const nsACString& aHeader, nsIHttpHeaderVisitor* aVisitor) override;
  NS_IMETHOD VisitOriginalResponseHeaders(
      nsIHttpHeaderVisitor* aVisitor) override;
  NS_IMETHOD IsNoStoreResponse(bool* aResult) override;
  NS_IMETHOD IsNoCacheResponse(bool* aResult) override;

 private:
  ~AtDocumentChannel() = default;

  struct Header {
    nsCString mName;
    nsCString mValue;
  };

  nsTArray<Header> mHeaders;
  uint64_t mChannelId = 0;
};

NS_IMPL_ISUPPORTS(AtDocumentChannelBase, nsIHttpChannel, nsIChannel, nsIRequest)

NS_IMETHODIMP
AtDocumentChannel::GetChannelId(uint64_t* aId) {
  *aId = mChannelId;
  return NS_OK;
}

NS_IMETHODIMP
AtDocumentChannel::SetChannelId(uint64_t aId) {
  mChannelId = aId;
  return NS_OK;
}

NS_IMETHODIMP
AtDocumentChannel::GetRequestMethod(nsACString& aMethod) {
  aMethod.AssignLiteral("GET");
  return NS_OK;
}

NS_IMETHODIMP
AtDocumentChannel::GetReferrerInfo(nsIReferrerInfo** aReferrerInfo) {
  *aReferrerInfo = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
AtDocumentChannel::SetReferrerInfo(nsIReferrerInfo* aReferrerInfo) {
  return NS_OK;
}

NS_IMETHODIMP
AtDocumentChannel::GetTransferSize(uint64_t* aSize) {
  *aSize = 0;
  return NS_OK;
}

NS_IMETHODIMP
AtDocumentChannel::GetRequestSize(uint64_t* aSize) {
  *aSize = 0;
  return NS_OK;
}

NS_IMETHODIMP
AtDocumentChannel::GetDecodedBodySize(uint64_t* aSize) {
  *aSize = 0;
  return NS_OK;
}

NS_IMETHODIMP
AtDocumentChannel::GetEncodedBodySize(uint64_t* aSize) {
  *aSize = 0;
  return NS_OK;
}

NS_IMETHODIMP
AtDocumentChannel::GetAllowSTS(bool* aAllowSTS) {
  *aAllowSTS = false;
  return NS_OK;
}

NS_IMETHODIMP
AtDocumentChannel::GetResponseStatus(uint32_t* aStatus) {
  *aStatus = 200;
  return NS_OK;
}

NS_IMETHODIMP
AtDocumentChannel::GetResponseStatusText(nsACString& aStatusText) {
  aStatusText.AssignLiteral("OK");
  return NS_OK;
}

NS_IMETHODIMP
AtDocumentChannel::GetRequestSucceeded(bool* aSucceeded) {
  *aSucceeded = true;
  return NS_OK;
}

NS_IMETHODIMP
AtDocumentChannel::GetResponseHeader(const nsACString& aHeader,
                                     nsACString& aValue) {
  aValue.Truncate();
  for (const Header& header : mHeaders) {
    if (header.mName.EqualsIgnoreCase(aHeader)) {
      if (!aValue.IsEmpty()) {
        aValue.AppendLiteral(", ");
      }
      aValue.Append(header.mValue);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
AtDocumentChannel::SetResponseHeader(const nsACString& aHeader,
                                     const nsACString& aValue, bool aMerge) {
  for (Header& header : mHeaders) {
    if (!header.mName.EqualsIgnoreCase(aHeader)) {
      continue;
    }
    if (aMerge) {
      header.mValue.AppendLiteral(", ");
      header.mValue.Append(aValue);
    } else {
      header.mValue.Assign(aValue);
    }
    return NS_OK;
  }
  mHeaders.AppendElement(Header{CopyString(aHeader), CopyString(aValue)});
  return NS_OK;
}

NS_IMETHODIMP
AtDocumentChannel::VisitResponseHeaders(nsIHttpHeaderVisitor* aVisitor) {
  for (const Header& header : mHeaders) {
    nsresult rv = aVisitor->VisitHeader(header.mName, header.mValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

NS_IMETHODIMP
AtDocumentChannel::GetOriginalResponseHeader(
    const nsACString& aHeader, nsIHttpHeaderVisitor* aVisitor) {
  nsAutoCString value;
  nsresult rv = GetResponseHeader(aHeader, value);
  NS_ENSURE_SUCCESS(rv, rv);
  if (value.IsEmpty()) {
    return NS_OK;
  }
  return aVisitor->VisitHeader(aHeader, value);
}

NS_IMETHODIMP
AtDocumentChannel::VisitOriginalResponseHeaders(
    nsIHttpHeaderVisitor* aVisitor) {
  return VisitResponseHeaders(aVisitor);
}

NS_IMETHODIMP
AtDocumentChannel::IsNoStoreResponse(bool* aResult) {
  *aResult = true;
  return NS_OK;
}

NS_IMETHODIMP
AtDocumentChannel::IsNoCacheResponse(bool* aResult) {
  *aResult = true;
  return NS_OK;
}

}  // namespace

AtProtocolChannel::AtProtocolChannel(
    UniquePtr<SimpleChannelCallbacks>&& aCallbacks)
    : SimpleChannel(std::move(aCallbacks)) {}

already_AddRefed<nsIChannel> NewAtProtocolChannel(
    nsIURI* aURI, nsILoadInfo* aLoadInfo, AtRecordResolver* aResolver) {
  auto startAsyncRead = [](nsIStreamListener* aListener, nsIChannel*,
                           AtRecordResolver* aResolver) -> RequestOrReason {
    return aResolver->Start(aListener);
  };
  auto openContentStream = [](bool, nsIChannel*, AtRecordResolver*) {
    return Err(NS_ERROR_NOT_IMPLEMENTED);
  };

  auto callbacks = std::make_unique<
      SimpleChannelCallbacksImpl<decltype(startAsyncRead),
                                 decltype(openContentStream), AtRecordResolver>>(
      std::move(startAsyncRead), std::move(openContentStream), aResolver);

  RefPtr<AtProtocolChannel> inner = new AtProtocolChannel(std::move(callbacks));
  inner->SetURI(aURI);
  inner->SetLoadInfo(aLoadInfo);

  RefPtr<AtDocumentChannel> outer = new AtDocumentChannel(inner);
  nsCOMPtr<nsIRequest> outerRequest(static_cast<nsIRequest*>(outer.get()));
  aResolver->SetOuterRequest(outerRequest);
  return outer.forget();
}

}  // namespace mozilla::net::atproto
