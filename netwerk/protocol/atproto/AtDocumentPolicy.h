/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef netwerk_protocol_atproto_AtDocumentPolicy_h
#define netwerk_protocol_atproto_AtDocumentPolicy_h

#include <string>
#include <string_view>

namespace mozilla::net::atproto {

extern const char kAtDocumentCsp[];
extern const char kAtDocumentIcon[];

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

struct DocumentHeader {
  std::string_view mRequested;
  std::string_view mDid;
  std::string_view mPds;
  std::string_view mCollection;
  std::string_view mRecordKey;
  std::string_view mCid;
  std::string_view mCidStatus;
};

ViewStyleOptions LoadViewStyleOptions();

void AppendDocumentSecurityMeta(std::string& aHtml);
void AppendDocumentIcon(std::string& aHtml);
void AppendDocumentStyle(std::string& aHtml,
                         const ViewStyleOptions& aOptions = {});
void AppendDocumentHeader(std::string& aHtml, const DocumentHeader& aHeader);

}  // namespace mozilla::net::atproto

#endif  // netwerk_protocol_atproto_AtDocumentPolicy_h
