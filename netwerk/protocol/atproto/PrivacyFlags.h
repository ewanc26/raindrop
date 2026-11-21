/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef netwerk_protocol_atproto_PrivacyFlags_h
#define netwerk_protocol_atproto_PrivacyFlags_h

#include <cstdint>

namespace mozilla::net::atproto {

uint32_t ResolutionLoadFlags(uint32_t aBaseFlags);

}  // namespace mozilla::net::atproto

#endif  // netwerk_protocol_atproto_PrivacyFlags_h
