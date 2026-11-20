/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef netwerk_protocol_atproto_AtURI_h
#define netwerk_protocol_atproto_AtURI_h

#include <string>
#include <string_view>

namespace mozilla::net::atproto {

enum class AtURIError {
  None,
  TooLong,
  InvalidScheme,
  EmptyAuthority,
  InvalidAuthority,
  InvalidCollection,
  InvalidRecordKey,
  InvalidPath,
};

const char* AtURIErrorName(AtURIError aError);

struct AtURI final {
  std::string mAuthority;
  std::string mCollection;
  std::string mRecordKey;
  std::string mFragment;
  bool mHasCollection = false;
  bool mHasRecordKey = false;
  bool mHasFragment = false;
};

AtURIError ParseAtURI(std::string_view aSpec, AtURI& aResult);
std::string NormalizeAtURI(std::string_view aSpec, AtURIError& aError);

bool IsValidHandle(std::string_view aHandle);
bool IsValidDid(std::string_view aDid);
bool IsValidNsid(std::string_view aNsid);
bool IsValidRecordKey(std::string_view aRecordKey);

}  // namespace mozilla::net::atproto

#endif
