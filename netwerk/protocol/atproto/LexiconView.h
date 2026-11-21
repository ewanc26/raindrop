/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef netwerk_protocol_atproto_LexiconView_h
#define netwerk_protocol_atproto_LexiconView_h

#include <string>
#include <string_view>

namespace mozilla::net::atproto {

struct LexiconViewProvenance {
  std::string_view mDid;
  std::string_view mPds;
  bool mShowImages = true;
};

std::string RenderLexiconSummary(std::string_view aLexicon,
                                 std::string_view aValueJson,
                                 const LexiconViewProvenance& aProvenance = {});

}  // namespace mozilla::net::atproto

#endif  // netwerk_protocol_atproto_LexiconView_h
