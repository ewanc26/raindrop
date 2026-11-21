/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef netwerk_protocol_atproto_CidVerifier_h
#define netwerk_protocol_atproto_CidVerifier_h

#include <string_view>

#include "json/json.h"

namespace mozilla::net::atproto {

enum class CidStatus {
  Absent,
  Verified,
  Mismatch,
  InvalidCid,
  UnsupportedValue,
};

const char* CidStatusName(CidStatus aStatus);

CidStatus VerifyRecordCid(std::string_view aCid, const Json::Value& aValue);

}  // namespace mozilla::net::atproto

#endif  // netwerk_protocol_atproto_CidVerifier_h
