/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef netwerk_protocol_atproto_HandleResolver_h
#define netwerk_protocol_atproto_HandleResolver_h

#include <string>
#include <string_view>

namespace mozilla::net::atproto {

enum class HandleError {
  None,
  InvalidHandle,
  InvalidResponse,
  TooLong,
};

const char* HandleErrorName(HandleError aError);

constexpr size_t kMaxHandleResponseLength = 8192;

// Build the HTTPS well-known URL that maps a handle to its DID.
HandleError BuildHandleDidUrl(std::string_view aHandle, std::string& aUrl);

// Parse the plain-text body of an atproto-did response into a supported DID.
HandleError ParseHandleDidResponse(std::string_view aBody, std::string& aDid);

}  // namespace mozilla::net::atproto

#endif  // netwerk_protocol_atproto_HandleResolver_h
