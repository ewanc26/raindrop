/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef netwerk_protocol_atproto_AtRecordResolver_h
#define netwerk_protocol_atproto_AtRecordResolver_h

#include <cstdint>
#include <string_view>

#include "AtURI.h"
#include "ErrorView.h"
#include "SimpleChannel.h"
#include "nsCOMPtr.h"
#include "nsICancelable.h"
#include "nsIChannelEventSink.h"
#include "nsIDNSListener.h"
#include "nsIInterfaceRequestor.h"
#include "nsIStreamListener.h"
#include "nsISupportsImpl.h"
#include "nsString.h"

class nsIInputStream;
class nsILoadInfo;
class nsIRequest;

namespace mozilla {
namespace net {
namespace atproto {

class AtRecordResolverTestAccess;

bool IsSameOrigin(std::string_view aSchemeA, std::string_view aHostA,
                  int32_t aPortA, std::string_view aSchemeB,
                  std::string_view aHostB, int32_t aPortB);

// Drives the lookups behind an at:// navigation. DID authorities take two
// steps; handle authorities first resolve the handle to a DID via DNS TXT at
// _atproto.<handle> (falling back to the HTTPS well-known route), then:
//   1. GET the DID document (did:plc / did:web), verifying alsoKnownAs for
//      handle authorities.
//   2. GET com.atproto.repo.getRecord from the PDS named in that document.
//
// It is the context object of NS_NewSimpleChannel. The at:// channel itself
// (a SimpleChannel/nsBaseChannel) owns the public nsIChannel behaviour; this
// class implements the internal stream and DNS listeners and an nsICancelable
// so a cancel of the outer channel reaches whichever lookup is in flight.
//
// The consumer sees exactly one OnStartRequest/OnDataAvailable/OnStopRequest
// sequence carrying the final record JSON, with the at:// channel as the
// request. Intermediate responses are consumed internally and never exposed.
class AtRecordResolver final : public nsIStreamListener,
                               public nsIDNSListener,
                               public nsICancelable,
                               public nsIInterfaceRequestor,
                               public nsIChannelEventSink {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIDNSLISTENER
  NS_DECL_NSICANCELABLE
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSICHANNELEVENTSINK

  AtRecordResolver(const AtURI& aUri, nsILoadInfo* aLoadInfo);

  // Begin resolution. aTarget is the at:// channel's listener (the
  // nsBaseChannel passed to StartAsyncRead). Synchronous setup errors are
  // returned so nsBaseChannel can fail the AsyncOpen. On success this object
  // is returned as the operation's nsICancelable.
  RequestOrReason Start(nsIStreamListener* aTarget);

 private:
  ~AtRecordResolver() = default;
  friend class AtRecordResolverTestAccess;

  enum class Phase { None, Handle, DidDocument, Record, Repo };

  nsresult StartResolution();
  nsresult StartHandleDnsLookup();
  nsresult StartHandleHttpLookup();
  nsresult StartDidDocumentFetch();
  nsresult StartFetch(const std::string& aUrl, Phase aPhase);
  nsresult CheckResponse(nsIRequest* aRequest, nsresult aStatus);
  nsresult OnHandleStop(nsIRequest* aRequest, nsresult aStatus);
  nsresult OnDidDocumentStop(nsIRequest* aRequest, nsresult aStatus);
  nsresult OnRecordStop(nsIRequest* aRequest, nsresult aStatus);
  nsresult OnRepoStop(nsIRequest* aRequest, nsresult aStatus);

  void SetFailure(FailureKind aKind, std::string_view aDetail);
  nsresult DeliverHtml(std::string_view aHtml);
  void DeliverFailure(nsresult aStatus);
  void ForwardStart();
  void ForwardStop(nsresult aStatus);

  nsCOMPtr<nsIRequest> mOuterRequest;
  nsCString mExpectedScheme;
  nsCString mExpectedHost;
  int32_t mExpectedPort = -1;
  nsCString mBody;
  std::string mDid;
  // Non-empty only when the authority is a handle; drives reverse binding.
  std::string mHandle;
  std::string mPds;
  FailureInfo mFailure;

  AtURI mUri;
  nsCOMPtr<nsILoadInfo> mLoadInfo;
  nsCOMPtr<nsIStreamListener> mTarget;
  nsCOMPtr<nsIRequest> mCurrentRequest;
  nsCOMPtr<nsICancelable> mDnsRequest;
  Phase mPhase = Phase::None;
  bool mDnsCompleted = false;
  bool mDeliveredStart = false;
  bool mDeliveredStop = false;
  bool mCanceled = false;
  bool mRedirectRejected = false;
};

}  // namespace atproto
}  // namespace net
}  // namespace mozilla

#endif  // netwerk_protocol_atproto_AtRecordResolver_h
