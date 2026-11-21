/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "AtDocumentPolicy.h"

namespace mozilla::net::atproto {

const char kAtDocumentCsp[] =
    "default-src 'none'; script-src 'none'; style-src 'unsafe-inline'; "
    "img-src https:; object-src 'none'; base-uri 'none'; form-action 'none'; "
    "frame-ancestors 'none'";

void AppendDocumentSecurityMeta(std::string& aHtml) {
  aHtml += "<meta http-equiv=\"Content-Security-Policy\" content=\"";
  aHtml += kAtDocumentCsp;
  aHtml += "\">\n<meta name=\"referrer\" content=\"no-referrer\">\n";
}

}  // namespace mozilla::net::atproto
