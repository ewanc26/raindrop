/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef netwerk_protocol_atproto_AtProtocolChannel_h
#define netwerk_protocol_atproto_AtProtocolChannel_h

#include "SimpleChannel.h"
#include "nsCOMPtr.h"

class nsIChannel;

namespace mozilla::net::atproto {

class AtRecordResolver;

class AtProtocolChannel final : public SimpleChannel {
 public:
  explicit AtProtocolChannel(UniquePtr<SimpleChannelCallbacks>&& aCallbacks);

 private:
  ~AtProtocolChannel() = default;
};

already_AddRefed<nsIChannel> NewAtProtocolChannel(
    nsIURI* aURI, nsILoadInfo* aLoadInfo, AtRecordResolver* aResolver);

}  // namespace mozilla::net::atproto

#endif  // netwerk_protocol_atproto_AtProtocolChannel_h
