/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "AtRecordResolver.h"

#include <string>

#include "gtest/gtest.h"
#include "mozilla/Maybe.h"
#include "nsCOMPtr.h"
#include "nsContentUtils.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsIChannel.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsStreamUtils.h"

using namespace mozilla::net::atproto;

namespace mozilla::net::atproto {

class AtRecordResolverTestAccess {
 public:
  static void SetExpectedOrigin(AtRecordResolver& aResolver, nsIURI* aUri) {
    aUri->GetScheme(aResolver.mExpectedScheme);
    aUri->GetHost(aResolver.mExpectedHost);
    aUri->GetPort(&aResolver.mExpectedPort);
  }

  static void SetRecord(AtRecordResolver& aResolver,
                        const std::string& aBody) {
    aResolver.mPhase = AtRecordResolver::Phase::Record;
    aResolver.mBody.Assign(aBody.c_str());
  }

  static void SetDidDocument(AtRecordResolver& aResolver, const std::string& aDid,
                             const std::string& aBody) {
    aResolver.mDid = aDid;
    aResolver.mPhase = AtRecordResolver::Phase::DidDocument;
    aResolver.mBody.Assign(aBody.c_str());
  }

  static const FailureInfo& Failure(const AtRecordResolver& aResolver) {
    return aResolver.mFailure;
  }

  static nsresult DidDocumentStop(AtRecordResolver& aResolver,
                                  nsIRequest* aRequest, nsresult aStatus) {
    return aResolver.OnDidDocumentStop(aRequest, aStatus);
  }

  static void SetTarget(AtRecordResolver& aResolver,
                        nsIStreamListener* aTarget, nsIRequest* aOuter) {
    aResolver.mTarget = aTarget;
    aResolver.mOuterRequest = aOuter;
  }

  static bool RedirectRejected(const AtRecordResolver& aResolver) {
    return aResolver.mRedirectRejected;
  }
};

}  // namespace mozilla::net::atproto

namespace {

already_AddRefed<AtRecordResolver> MakeResolver(const char* aAuthority) {
  AtURI uri;
  uri.mAuthority = aAuthority;
  return do_AddRef(new AtRecordResolver(uri, nullptr));
}

already_AddRefed<nsIChannel> MakeChannel(const nsACString& aSpec) {
  nsCOMPtr<nsIURI> uri;
  if (NS_FAILED(NS_NewURI(getter_AddRefs(uri), aSpec))) {
    return nullptr;
  }
  nsCOMPtr<nsIChannel> channel;
  nsresult rv = NS_NewChannel(
      getter_AddRefs(channel), uri, nsContentUtils::GetSystemPrincipal(),
      nsILoadInfo::SEC_ALLOW_CROSS_ORIGIN_SEC_CONTEXT_IS_NULL,
      nsIContentPolicy::TYPE_OTHER);
  if (NS_FAILED(rv)) {
    return nullptr;
  }
  return channel.forget();
}

class StubRedirectCallback final : public nsIAsyncVerifyRedirectCallback {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIASYNCVERIFYREDIRECTCALLBACK

  nsresult mResult = NS_OK;
  int mCalls = 0;

 private:
  ~StubRedirectCallback() = default;
};

NS_IMPL_ISUPPORTS(StubRedirectCallback, nsIAsyncVerifyRedirectCallback)

NS_IMETHODIMP
StubRedirectCallback::OnRedirectVerifyCallback(nsresult aResult) {
  mResult = aResult;
  ++mCalls;
  return NS_OK;
}

class RecordingListener final : public nsIStreamListener {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER

  bool mStarted = false;
  nsCString mData;
  mozilla::Maybe<nsresult> mStop;

 private:
  ~RecordingListener() = default;
};

NS_IMPL_ISUPPORTS(RecordingListener, nsIStreamListener, nsIRequestObserver)

NS_IMETHODIMP
RecordingListener::OnStartRequest(nsIRequest*) {
  mStarted = true;
  return NS_OK;
}

NS_IMETHODIMP
RecordingListener::OnDataAvailable(nsIRequest*, nsIInputStream* aStream,
                                   uint64_t, uint32_t aCount) {
  nsAutoCString chunk;
  nsresult rv = NS_ConsumeStream(aStream, aCount, chunk);
  if (NS_SUCCEEDED(rv)) {
    mData.Append(chunk);
  }
  return rv;
}

NS_IMETHODIMP
RecordingListener::OnStopRequest(nsIRequest*, nsresult aStatus) {
  mStop = mozilla::Some(aStatus);
  return NS_OK;
}

void ExpectOriginFromSpec(AtRecordResolver& aResolver,
                          const nsACString& aSpec) {
  nsCOMPtr<nsIURI> uri;
  ASSERT_TRUE(NS_SUCCEEDED(NS_NewURI(getter_AddRefs(uri), aSpec)));
  AtRecordResolverTestAccess::SetExpectedOrigin(aResolver, uri);
}

constexpr const char* kRecordBody = R"({"value":{"text":"secret"}})";

}  // namespace

TEST(AtRecordResolver, SameOriginAllowsDefaultPortSpelling) {
  EXPECT_TRUE(IsSameOrigin("https", "pds.example.com", -1,
                           "https", "pds.example.com", 443));
}

TEST(AtRecordResolver, SameOriginIgnoresAsciiCase) {
  EXPECT_TRUE(IsSameOrigin("https", "PDS.Example.COM", -1,
                           "HTTPS", "pds.example.com", -1));
}

TEST(AtRecordResolver, RejectsSchemeDowngrade) {
  EXPECT_FALSE(IsSameOrigin("https", "pds.example.com", -1,
                            "http", "pds.example.com", -1));
}

TEST(AtRecordResolver, RejectsPortChange) {
  EXPECT_FALSE(IsSameOrigin("https", "pds.example.com", -1,
                            "https", "pds.example.com", 8443));
}

TEST(AtRecordResolver, RejectsHostChange) {
  EXPECT_FALSE(IsSameOrigin("https", "pds.example.com", -1,
                            "https", "evil.example.com", -1));
}

TEST(AtRecordResolver, ExposesChannelEventSink) {
  RefPtr<AtRecordResolver> resolver = MakeResolver("did:plc:abc");
  void* raw = nullptr;
  nsresult rv = resolver->GetInterface(NS_GET_IID(nsIChannelEventSink), &raw);
  EXPECT_TRUE(NS_SUCCEEDED(rv));
  auto* sink = static_cast<nsIChannelEventSink*>(raw);
  ASSERT_TRUE(sink);
  NS_RELEASE(sink);
}

TEST(AtRecordResolver, SinkVetoesCrossOriginRedirectHop) {
  RefPtr<AtRecordResolver> resolver = MakeResolver("did:plc:abc");
  ExpectOriginFromSpec(*resolver, "https://pds.example.com/x"_ns);

  nsCOMPtr<nsIChannel> oldChannel = MakeChannel("https://pds.example.com/x"_ns);
  nsCOMPtr<nsIChannel> newChannel =
      MakeChannel("https://attacker.example/x"_ns);
  ASSERT_TRUE(oldChannel);
  ASSERT_TRUE(newChannel);

  RefPtr<StubRedirectCallback> callback = new StubRedirectCallback();
  nsresult rv = resolver->AsyncOnChannelRedirect(oldChannel, newChannel, 0,
                                                 callback);
  EXPECT_TRUE(NS_SUCCEEDED(rv));
  EXPECT_EQ(callback->mCalls, 1);
  EXPECT_EQ(callback->mResult, NS_ERROR_NOT_AVAILABLE);
  EXPECT_TRUE(AtRecordResolverTestAccess::RedirectRejected(*resolver));
}

TEST(AtRecordResolver, SinkAllowsSameOriginRedirectHop) {
  RefPtr<AtRecordResolver> resolver = MakeResolver("did:plc:abc");
  ExpectOriginFromSpec(*resolver, "https://pds.example.com/x"_ns);

  nsCOMPtr<nsIChannel> oldChannel = MakeChannel("https://pds.example.com/x"_ns);
  nsCOMPtr<nsIChannel> newChannel = MakeChannel("https://pds.example.com/y"_ns);
  ASSERT_TRUE(oldChannel);
  ASSERT_TRUE(newChannel);

  RefPtr<StubRedirectCallback> callback = new StubRedirectCallback();
  nsresult rv = resolver->AsyncOnChannelRedirect(oldChannel, newChannel, 0,
                                                 callback);
  EXPECT_TRUE(NS_SUCCEEDED(rv));
  EXPECT_EQ(callback->mCalls, 1);
  EXPECT_EQ(callback->mResult, NS_OK);
  EXPECT_FALSE(AtRecordResolverTestAccess::RedirectRejected(*resolver));
}

TEST(AtRecordResolver, CanceledCompletionDoesNotDeliverRecord) {
  RefPtr<AtRecordResolver> resolver = MakeResolver("did:plc:abc");
  RefPtr<RecordingListener> listener = new RecordingListener();
  nsCOMPtr<nsIChannel> outer = MakeChannel("https://pds.example.com/record"_ns);
  ASSERT_TRUE(outer);
  AtRecordResolverTestAccess::SetTarget(*resolver, listener, outer);
  AtRecordResolverTestAccess::SetRecord(*resolver, kRecordBody);
  ExpectOriginFromSpec(*resolver, "https://pds.example.com/record"_ns);

  resolver->Cancel(NS_BINDING_ABORTED);
  nsresult rv = resolver->OnStopRequest(outer, NS_OK);
  EXPECT_TRUE(NS_SUCCEEDED(rv));
  EXPECT_TRUE(listener->mStarted);
  EXPECT_TRUE(listener->mData.IsEmpty());
  ASSERT_TRUE(listener->mStop);
  EXPECT_EQ(*listener->mStop, NS_OK);
}

TEST(AtRecordResolver, OnionEndpointKeepsProvenanceAndSpecificFailure) {
  RefPtr<AtRecordResolver> resolver = MakeResolver("did:plc:abc");
  const std::string json =
      R"({"id":"did:plc:abc","service":[{"id":"#atproto_pds",)"
      R"("serviceEndpoint":"https://abcdef123456.onion"}]})";
  AtRecordResolverTestAccess::SetDidDocument(*resolver, "did:plc:abc", json);

  nsresult rv =
      AtRecordResolverTestAccess::DidDocumentStop(*resolver, nullptr, NS_OK);
  EXPECT_EQ(rv, NS_ERROR_NOT_AVAILABLE);

  const FailureInfo& failure = AtRecordResolverTestAccess::Failure(*resolver);
  EXPECT_EQ(failure.mPds, "https://abcdef123456.onion");
  std::string html = RenderFailureHtml(failure);
  EXPECT_NE(html.find("onion"), std::string::npos);
  EXPECT_NE(html.find("Tor"), std::string::npos);
}

TEST(AtRecordResolver, CompletionDeliversRecordWhenNotCanceled) {
  RefPtr<AtRecordResolver> resolver = MakeResolver("did:plc:abc");
  RefPtr<RecordingListener> listener = new RecordingListener();
  nsCOMPtr<nsIChannel> outer = MakeChannel("https://pds.example.com/record"_ns);
  ASSERT_TRUE(outer);
  AtRecordResolverTestAccess::SetTarget(*resolver, listener, outer);
  AtRecordResolverTestAccess::SetRecord(*resolver, kRecordBody);
  ExpectOriginFromSpec(*resolver, "https://pds.example.com/record"_ns);

  nsresult rv = resolver->OnStopRequest(outer, NS_OK);
  EXPECT_TRUE(NS_SUCCEEDED(rv));
  EXPECT_TRUE(listener->mStarted);
  EXPECT_FALSE(listener->mData.IsEmpty());
  EXPECT_NE(listener->mData.Find("secret"_ns), size_t(-1));
  ASSERT_TRUE(listener->mStop);
  EXPECT_EQ(*listener->mStop, NS_OK);
}
