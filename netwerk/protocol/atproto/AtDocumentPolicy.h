/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef netwerk_protocol_atproto_AtDocumentPolicy_h
#define netwerk_protocol_atproto_AtDocumentPolicy_h

#include <string>

namespace mozilla::net::atproto {

extern const char kAtDocumentCsp[];

void AppendDocumentSecurityMeta(std::string& aHtml);

}  // namespace mozilla::net::atproto

#endif  // netwerk_protocol_atproto_AtDocumentPolicy_h
