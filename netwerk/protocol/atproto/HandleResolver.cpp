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
    case HandleError::TxtNoDid:
      return "TxtNoDid";
    case HandleError::TxtMultipleDid:
      return "TxtMultipleDid";
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

HandleError BuildHandleTxtHost(std::string_view aHandle, std::string& aHost) {
  aHost.clear();
  if (!IsValidHandle(aHandle)) {
    return HandleError::InvalidHandle;
  }
  aHost = "_atproto.";
  aHost += aHandle;
  return HandleError::None;
}

HandleError ParseHandleTxtRecords(const std::vector<std::string>& aRecords,
                                  std::string& aDid) {
  aDid.clear();
  constexpr std::string_view prefix = "did=";

  constexpr std::string_view whitespace = " \t\r\n";
  std::vector<std::string_view> claims;
  for (const std::string& record : aRecords) {
    if (record.size() > kMaxHandleResponseLength) {
      return HandleError::TooLong;
    }
    std::string_view trimmed(record);
    auto first = trimmed.find_first_not_of(whitespace);
    if (first == std::string_view::npos) {
      continue;
    }
    auto last = trimmed.find_last_not_of(whitespace);
    trimmed = trimmed.substr(first, last - first + 1);
    if (trimmed.substr(0, prefix.size()) == prefix) {
      claims.push_back(trimmed.substr(prefix.size()));
    }
  }

  if (claims.empty()) {
    return HandleError::TxtNoDid;
  }
  // Count every did= record before validating any, so a malformed first claim
  // cannot hide a second one and slip through the ambiguity check.
  if (claims.size() > 1) {
    return HandleError::TxtMultipleDid;
  }
  Did did;
  if (ParseDid(claims.front(), did) != DidError::None) {
    return HandleError::InvalidResponse;
  }
  aDid = did.mFull;
  return HandleError::None;
}

}  // namespace mozilla::net::atproto
