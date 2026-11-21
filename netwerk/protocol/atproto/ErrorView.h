/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef netwerk_protocol_atproto_ErrorView_h
#define netwerk_protocol_atproto_ErrorView_h

#include <string>

namespace mozilla::net::atproto {

enum class FailureKind {
  HandleResolutionFailed,
  HandleBindingFailed,
  DidResolutionFailed,
  DidDocumentRejected,
  PdsUnreachable,
  RecordNotFound,
  RecordInvalid,
  UpstreamRejected,
  RedirectRejected,
  NetworkError,
};

struct FailureInfo {
  FailureKind mKind = FailureKind::NetworkError;
  std::string mAuthority;
  std::string mDid;
  std::string mPds;
  std::string mDetail;
};

const char* FailureTitle(FailureKind aKind);
const char* FailureExplanation(FailureKind aKind);
std::string RenderFailureHtml(const FailureInfo& aInfo);

}  // namespace mozilla::net::atproto

#endif  // netwerk_protocol_atproto_ErrorView_h
