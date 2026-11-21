/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "AtProtocolHandler.h"

#include "AtRecordResolver.h"
#include "AtURI.h"
#include "SimpleChannel.h"
#include "mozilla/Preferences.h"
#include "mozilla/StaticMutex.h"
#include "mozilla/StaticPtr.h"
#include "nsIChannel.h"
#include "nsILoadInfo.h"

namespace mozilla::net::atproto {

NS_IMPL_ISUPPORTS(AtProtocolHandler, nsIProtocolHandler,
                  nsISupportsWeakReference)

static StaticRefPtr<AtProtocolHandler> sSingleton;
static StaticMutex sSingletonLock;

already_AddRefed<AtProtocolHandler> AtProtocolHandler::GetSingleton() {
  StaticMutexAutoLock lock(sSingletonLock);
  if (!sSingleton) {
    sSingleton = new AtProtocolHandler();
  }
  return do_AddRef(sSingleton);
}

NS_IMETHODIMP
AtProtocolHandler::GetScheme(nsACString& aScheme) {
  aScheme.AssignLiteral("at");
  return NS_OK;
}

NS_IMETHODIMP
AtProtocolHandler::AllowPort(int32_t aPort, const char* aScheme,
                             bool* aRetval) {
  *aRetval = false;
  return NS_OK;
}

NS_IMETHODIMP
AtProtocolHandler::NewChannel(nsIURI* aURI, nsILoadInfo* aLoadInfo,
                              nsIChannel** aResult) {
  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nullptr;

  if (!Preferences::GetBool("network.atprotocol.enabled", true)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsAutoCString spec;
  nsresult rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  AtURI parsed;
  AtURIError error =
      ParseAtURI(std::string_view(spec.BeginReading(), spec.Length()), parsed);
  if (error != AtURIError::None) {
    return NS_ERROR_MALFORMED_URI;
  }

  RefPtr<AtRecordResolver> resolver = new AtRecordResolver(parsed, aLoadInfo);
  nsCOMPtr<nsIChannel> channel =
      NS_NewSimpleChannel(aURI, aLoadInfo, resolver.get(),
                          [](nsIStreamListener* aListener, nsIChannel*,
                             AtRecordResolver* aResolver) -> RequestOrReason {
                            return aResolver->Start(aListener);
                          });
  NS_ENSURE_TRUE(channel, NS_ERROR_OUT_OF_MEMORY);

  // The resolver renders record, repository and collection responses into HTML.
  rv = channel->SetContentType("text/html"_ns);
  NS_ENSURE_SUCCESS(rv, rv);

  channel.forget(aResult);
  return NS_OK;
}

}  // namespace mozilla::net::atproto
