/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef netwerk_protocol_atproto_AtProtocolHandler_h
#define netwerk_protocol_atproto_AtProtocolHandler_h

#include "nsIProtocolHandler.h"
#include "nsIURI.h"
#include "nsString.h"
#include "nsWeakReference.h"

namespace mozilla::net::atproto {

class AtProtocolHandler final : public nsIProtocolHandler,
                                public nsSupportsWeakReference {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROTOCOLHANDLER

  AtProtocolHandler() = default;

  static already_AddRefed<AtProtocolHandler> GetSingleton();

 private:
  ~AtProtocolHandler() = default;
};

}  // namespace mozilla::net::atproto

#endif
