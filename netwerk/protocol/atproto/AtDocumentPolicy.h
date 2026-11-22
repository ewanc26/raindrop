/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef netwerk_protocol_atproto_AtDocumentPolicy_h
#define netwerk_protocol_atproto_AtDocumentPolicy_h

#include <string>

namespace mozilla::net::atproto {

extern const char kAtDocumentCsp[];

enum class ViewTheme { System, Light, Dark, HighContrast };
enum class ViewTextSize { Small, Medium, Large, ExtraLarge };
enum class ViewWidth { Narrow, Standard, Wide, Full };
enum class ViewAccent { Blue, Green, Purple, Amber, Slate };

struct ViewStyleOptions {
  ViewTheme mTheme = ViewTheme::System;
  ViewTextSize mTextSize = ViewTextSize::Medium;
  ViewWidth mWidth = ViewWidth::Standard;
  ViewAccent mAccent = ViewAccent::Blue;
  bool mCompact = false;
};

ViewStyleOptions LoadViewStyleOptions();

void AppendDocumentSecurityMeta(std::string& aHtml);
void AppendDocumentStyle(std::string& aHtml,
                         const ViewStyleOptions& aOptions = {});

}  // namespace mozilla::net::atproto

#endif  // netwerk_protocol_atproto_AtDocumentPolicy_h
