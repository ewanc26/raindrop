/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef netwerk_protocol_atproto_HandleResolver_h
#define netwerk_protocol_atproto_HandleResolver_h

#include <string>
#include <string_view>
#include <vector>

namespace mozilla::net::atproto {

enum class HandleError {
  None,
  InvalidHandle,
  InvalidResponse,
  TooLong,
  TxtNoDid,
  TxtMultipleDid,
};

const char* HandleErrorName(HandleError aError);

constexpr size_t kMaxHandleResponseLength = 8192;

// Build the HTTPS well-known URL that maps a handle to its DID.
HandleError BuildHandleDidUrl(std::string_view aHandle, std::string& aUrl);

// Parse the plain-text body of an atproto-did response into a supported DID.
HandleError ParseHandleDidResponse(std::string_view aBody, std::string& aDid);

// Build the DNS TXT query host for a handle, per ADR 0003:
//   _atproto.<handle>
HandleError BuildHandleTxtHost(std::string_view aHandle, std::string& aHost);

// Select a DID from the strings of a DNS TXT record at _atproto.<handle>.
// atproto requires exactly one record beginning with "did="; zero is
// TxtNoDid, more than one is TxtMultipleDid (ambiguous, never guessed).
HandleError ParseHandleTxtRecords(const std::vector<std::string>& aRecords,
                                  std::string& aDid);

}  // namespace mozilla::net::atproto

#endif  // netwerk_protocol_atproto_HandleResolver_h
