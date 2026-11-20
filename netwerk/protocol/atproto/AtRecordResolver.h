/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef netwerk_protocol_atproto_AtRecordResolver_h
#define netwerk_protocol_atproto_AtRecordResolver_h

#include "AtURI.h"
#include "SimpleChannel.h"
#include "nsCOMPtr.h"
#include "nsICancelable.h"
#include "nsIStreamListener.h"
#include "nsString.h"

class nsIInputStream;
class nsILoadInfo;
class nsIRequest;

namespace mozilla {
namespace net {
namespace atproto {

// Drives the two sequential HTTPS fetches behind an at:// navigation:
//   1. GET the DID document for the URI's authority (did:plc / did:web).
//   2. GET com.atproto.repo.getRecord from the PDS named in that document.
//
// It is the context object of NS_NewSimpleChannel. The at:// channel itself
// (a SimpleChannel/nsBaseChannel) owns the public nsIChannel behaviour; this
// class implements the internal stream listeners and an nsICancelable so a
// cancel of the outer channel reaches whichever HTTP request is in flight.
//
// The consumer sees exactly one OnStartRequest/OnDataAvailable/OnStopRequest
// sequence carrying the final record JSON, with the at:// channel as the
// request. The DID-document response is consumed internally and never exposed.
class AtRecordResolver final : public nsIStreamListener, public nsICancelable {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSICANCELABLE

  AtRecordResolver(const AtURI& aUri, nsILoadInfo* aLoadInfo);

  // Begin the DID-document fetch. aTarget is the at:// channel's listener
  // (the nsBaseChannel passed to StartAsyncRead). Synchronous setup errors
  // are returned so nsBaseChannel can fail the AsyncOpen. On success this
  // object is returned as the operation's nsICancelable.
  RequestOrReason Start(nsIStreamListener* aTarget);

 private:
  ~AtRecordResolver() = default;

  enum class Phase { None, DidDocument, Record };

  nsresult StartDidDocumentFetch();
  nsresult StartFetch(const std::string& aUrl, Phase aPhase);
  nsresult OnDidDocumentStop(nsIRequest* aRequest, nsresult aStatus);

  void DeliverFailure(nsresult aStatus);
  void ForwardStart();
  void ForwardStop(nsresult aStatus);

  nsCOMPtr<nsIRequest> mOuterRequest;
  nsCString mExpectedHost;
  nsCString mBody;
  std::string mDid;

  AtURI mUri;
  nsCOMPtr<nsILoadInfo> mLoadInfo;
  nsCOMPtr<nsIStreamListener> mTarget;
  nsCOMPtr<nsIRequest> mCurrentRequest;
  Phase mPhase = Phase::None;
  bool mDeliveredStart = false;
  bool mDeliveredStop = false;
  bool mCanceled = false;
};

}  // namespace atproto
}  // namespace net
}  // namespace mozilla

#endif  // netwerk_protocol_atproto_AtRecordResolver_h
