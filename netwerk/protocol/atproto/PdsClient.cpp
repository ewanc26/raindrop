/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "PdsClient.h"

#include "AtURI.h"
#include "DidResolver.h"

namespace mozilla::net::atproto {

namespace {

bool IsUnreserved(char aChar) {
  return (aChar >= 'a' && aChar <= 'z') || (aChar >= 'A' && aChar <= 'Z') ||
         (aChar >= '0' && aChar <= '9') || aChar == '.' || aChar == '-' ||
         aChar == '_' || aChar == '~';
}

char HexDigit(int aValue) {
  return aValue < 10 ? '0' + aValue : 'A' + aValue - 10;
}

// Remove trailing slashes so that joining with "/xrpc/..." never creates a
// double slash. The serviceEndpoint is an origin in practice, but accept an
// explicit trailing slash for interoperability.
std::string_view TrimTrailingSlashes(std::string_view aValue) {
  while (!aValue.empty() && aValue.back() == '/') {
    aValue = aValue.substr(0, aValue.size() - 1);
  }
  return aValue;
}

}  // namespace

const char* PdsUrlErrorName(PdsUrlError aError) {
  switch (aError) {
    case PdsUrlError::None:
      return "None";
    case PdsUrlError::InvalidEndpoint:
      return "InvalidEndpoint";
    case PdsUrlError::InvalidRepo:
      return "InvalidRepo";
    case PdsUrlError::InvalidCollection:
      return "InvalidCollection";
    case PdsUrlError::InvalidRecordKey:
      return "InvalidRecordKey";
  }
  return "Unknown";
}

std::string EncodeQueryValue(std::string_view aValue) {
  std::string result;
  result.reserve(aValue.size());
  for (unsigned char c : aValue) {
    if (IsUnreserved(static_cast<char>(c))) {
      result.push_back(static_cast<char>(c));
      continue;
    }
    result.push_back('%');
    result.push_back(HexDigit(c >> 4));
    result.push_back(HexDigit(c & 0x0f));
  }
  return result;
}

PdsUrlError BuildGetRecordUrl(std::string_view aEndpoint,
                              std::string_view aRepo,
                              std::string_view aCollection,
                              std::string_view aRecordKey,
                              std::string& aUrl) {
  aUrl.clear();
  if (!IsValidPdsEndpoint(aEndpoint)) {
    return PdsUrlError::InvalidEndpoint;
  }
  if (!IsValidDid(aRepo)) {
    return PdsUrlError::InvalidRepo;
  }
  if (!IsValidNsid(aCollection)) {
    return PdsUrlError::InvalidCollection;
  }
  if (!IsValidRecordKey(aRecordKey)) {
    return PdsUrlError::InvalidRecordKey;
  }

  aUrl = std::string(TrimTrailingSlashes(aEndpoint)) +
         "/xrpc/com.atproto.repo.getRecord?repo=" + EncodeQueryValue(aRepo) +
         "&collection=" + EncodeQueryValue(aCollection) +
         "&rkey=" + EncodeQueryValue(aRecordKey);
  return PdsUrlError::None;
}

}  // namespace mozilla::net::atproto
