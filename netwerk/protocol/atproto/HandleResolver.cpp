/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "HandleResolver.h"

#include "AtURI.h"
#include "DidResolver.h"

namespace mozilla::net::atproto {

const char* HandleErrorName(HandleError aError) {
  switch (aError) {
    case HandleError::None:
      return "None";
    case HandleError::InvalidHandle:
      return "InvalidHandle";
    case HandleError::InvalidResponse:
      return "InvalidResponse";
    case HandleError::TooLong:
      return "TooLong";
  }
  return "Unknown";
}

HandleError BuildHandleDidUrl(std::string_view aHandle, std::string& aUrl) {
  aUrl.clear();
  if (!IsValidHandle(aHandle)) {
    return HandleError::InvalidHandle;
  }
  aUrl = "https://";
  aUrl += aHandle;
  aUrl += "/.well-known/atproto-did";
  return HandleError::None;
}

HandleError ParseHandleDidResponse(std::string_view aBody, std::string& aDid) {
  aDid.clear();
  if (aBody.size() > kMaxHandleResponseLength) {
    return HandleError::TooLong;
  }

  constexpr std::string_view whitespace = " \t\r\n";
  auto first = aBody.find_first_not_of(whitespace);
  if (first == std::string_view::npos) {
    return HandleError::InvalidResponse;
  }
  auto last = aBody.find_last_not_of(whitespace);
  auto trimmed = aBody.substr(first, last - first + 1);

  Did did;
  if (ParseDid(trimmed, did) != DidError::None) {
    return HandleError::InvalidResponse;
  }
  aDid = did.mFull;
  return HandleError::None;
}

}  // namespace mozilla::net::atproto
