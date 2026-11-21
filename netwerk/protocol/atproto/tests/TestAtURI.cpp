/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <vector>

#include "AtProtocolVectors.h"
#include "AtURI.h"
#include "gtest/gtest.h"

using namespace mozilla::net::atproto;

namespace {

void ExpectNormalized(std::string_view aInput, std::string_view aExpected) {
  AtURIError error = AtURIError::None;
  EXPECT_EQ(NormalizeAtURI(aInput, error), aExpected) << "for input " << aInput;
  EXPECT_EQ(error, AtURIError::None) << AtURIErrorName(error);
}

void ExpectNavigationNormalized(std::string_view aInput,
                                std::string_view aExpected) {
  AtURIError error = AtURIError::None;
  EXPECT_EQ(NormalizeNavigationAtURI(aInput, error), aExpected)
      << "for input " << aInput;
  EXPECT_EQ(error, AtURIError::None) << AtURIErrorName(error);
}

void ExpectError(std::string_view aInput, AtURIError aExpected) {
  AtURIError error = AtURIError::None;
  AtURI uri;
  error = ParseAtURI(aInput, uri);
  EXPECT_EQ(error, aExpected)
      << "for input " << aInput << ", got " << AtURIErrorName(error);
}

std::vector<std::string_view> CorpusLines(std::string_view aCorpus) {
  std::vector<std::string_view> lines;
  size_t start = 0;
  while (start <= aCorpus.size()) {
    size_t end = aCorpus.find('\n', start);
    if (end == std::string_view::npos) {
      end = aCorpus.size();
    }
    std::string_view line = aCorpus.substr(start, end - start);
    size_t first = line.find_first_not_of(" \t");
    if (first != std::string_view::npos && line[first] != '#') {
      lines.push_back(line);
    }
    if (end == aCorpus.size()) {
      break;
    }
    start = end + 1;
  }
  return lines;
}

template <typename Validator>
void ExpectPrimitiveCorpus(std::string_view aValid, std::string_view aInvalid,
                           Validator aValidator) {
  for (auto value : CorpusLines(aValid)) {
    EXPECT_TRUE(aValidator(value)) << "valid vector rejected: " << value;
  }
  for (auto value : CorpusLines(aInvalid)) {
    EXPECT_FALSE(aValidator(value)) << "invalid vector accepted: " << value;
  }
}

}  // namespace

TEST(AtURI, ValidRecordUris)
{
  ExpectNormalized(
      "at://did:plc:vwzwgnygau7ed7b7wt5ux7y2/app.bsky.feed.post/3k5nobkf2w72g",
      "at://did:plc:vwzwgnygau7ed7b7wt5ux7y2/app.bsky.feed.post/3k5nobkf2w72g");
  ExpectNormalized("at://retr0.id/app.bsky.feed.post/3k5nobkf2w72g",
                   "at://retr0.id/app.bsky.feed.post/3k5nobkf2w72g");
  ExpectNormalized(
      "at://did:web:user.example.com/com.atproto.lexicon.schema/"
      "com.atproto.lexicon.schema",
      "at://did:web:user.example.com/com.atproto.lexicon.schema/"
      "com.atproto.lexicon.schema");
  ExpectNormalized("at://foo.com/com.example.foo/123",
                   "at://foo.com/com.example.foo/123");
  ExpectNormalized("at://did:plc:x/app.bsky.feed.post",
                   "at://did:plc:x/app.bsky.feed.post");
  ExpectNormalized("at://foo.com", "at://foo.com");
}

TEST(AtURI, Normalization)
{
  ExpectNormalized("at://Jay.Bsky.Social/Com.EXAMPLE.FooBar/self",
                   "at://jay.bsky.social/com.example.FooBar/self");
}

TEST(AtURI, SchemeAndIdentifierNormalization)
{
  ExpectError("AT://did:plc:asdf123", AtURIError::InvalidScheme);
  ExpectError("at://DiD:plc:asdf123", AtURIError::InvalidAuthority);
  ExpectNormalized("at://did:web:User.Example.com%3a8443/com.example.foo/123",
                   "at://did:web:user.example.com%3A8443/com.example.foo/123");
  ExpectError("at://did:web:example.com%253A8443/com.example.foo/123",
              AtURIError::InvalidAuthority);
}

TEST(AtURI, InvalidSchemeAndAuthority)
{
  ExpectError("https://foo.com/com.example.foo/123", AtURIError::InvalidScheme);
  ExpectError("at:///com.example.foo/123", AtURIError::EmptyAuthority);
  ExpectError("at://", AtURIError::EmptyAuthority);
  ExpectError("at://computer", AtURIError::InvalidAuthority);
  ExpectError("at://example.com:3000", AtURIError::InvalidAuthority);
  ExpectError("at://user:pass@foo.com", AtURIError::InvalidAuthority);
  ExpectError("at://@foo.com", AtURIError::InvalidAuthority);
  ExpectError("at://foo..com/com.example.foo/123",
              AtURIError::InvalidAuthority);
  ExpectError("at://foo.0/com.example.foo/123", AtURIError::InvalidAuthority);
}

TEST(AtURI, InvalidDids)
{
  ExpectError("at://did:METHOD:val/com.example.foo/123",
              AtURIError::InvalidAuthority);
  ExpectError("at://did:m123:val/com.example.foo/123",
              AtURIError::InvalidAuthority);
  ExpectError("at://did:method:/com.example.foo/123",
              AtURIError::InvalidAuthority);
  ExpectError("at://did:method:val/two/com.example.foo/123",
              AtURIError::InvalidPath);
  ExpectError("at://did:method:val%/com.example.foo/123",
              AtURIError::InvalidAuthority);
  ExpectNormalized("at://did:m:v/com.example.foo/123",
                   "at://did:m:v/com.example.foo/123");
  ExpectNormalized("at://did:method::::val/com.example.foo/123",
                   "at://did:method::::val/com.example.foo/123");
}

TEST(AtURI, InvalidPaths)
{
  ExpectError("at://foo.com/", AtURIError::InvalidPath);
  ExpectError("at://did:plc:x/app.bsky.feed.post/", AtURIError::InvalidPath);
  ExpectError("at://did:plc:x//self", AtURIError::InvalidPath);
  ExpectError("at://did:plc:x/app.bsky.feed.post/./self",
              AtURIError::InvalidPath);
  ExpectError("at://did:plc:x/app.bsky.feed.post/../self",
              AtURIError::InvalidPath);
  ExpectError("at://did:plc:x/app.bsky.feed.post/self/extra",
              AtURIError::InvalidPath);
  ExpectError("at://did:plc:x/app.bsky.feed.post/self?x=1",
              AtURIError::InvalidPath);
  ExpectError("at://did:plc:x/app.bsky.feed.post/self#frag",
              AtURIError::InvalidPath);
  ExpectError("at://did:plc:x#", AtURIError::InvalidPath);
  ExpectError("at://did:plc:x/app.bsky.feed.post/self#frag#ment",
              AtURIError::InvalidPath);
}

TEST(AtURI, InvalidCollections)
{
  ExpectError("at://foo.com/example/123", AtURIError::InvalidCollection);
  ExpectError("at://foo.com/com.example/123", AtURIError::InvalidCollection);
  ExpectError("at://foo.com/com.example.3/123", AtURIError::InvalidCollection);
  ExpectError("at://foo.com/3com.example.foo/123",
              AtURIError::InvalidCollection);
  ExpectError("at://foo.com/com.example-foo/123",
              AtURIError::InvalidCollection);
  ExpectError("at://foo.com/com.example.foo%2F123/self",
              AtURIError::InvalidCollection);
  ExpectError("at://foo.com/%61pp.bsky.feed.post/self",
              AtURIError::InvalidCollection);
}

TEST(AtURI, InvalidRecordKeys)
{
  ExpectError("at://foo.com/com.example.foo/.", AtURIError::InvalidPath);
  ExpectError("at://foo.com/com.example.foo/..", AtURIError::InvalidPath);
  ExpectError("at://foo.com/com.example.foo/@handle",
              AtURIError::InvalidRecordKey);
  ExpectError("at://foo.com/com.example.foo/any+space",
              AtURIError::InvalidRecordKey);
  ExpectError("at://foo.com/com.example.foo/dHJ1ZQ==",
              AtURIError::InvalidRecordKey);
  ExpectError("at://foo.com/com.example.foo/%73elf",
              AtURIError::InvalidRecordKey);
  ExpectError("at://foo.com/com.example.foo/%30", AtURIError::InvalidRecordKey);
  ExpectNormalized("at://foo.com/com.example.foo/~1.2-3_",
                   "at://foo.com/com.example.foo/~1.2-3_");
}

TEST(AtURI, CursorSegmentsAreNavigationOnly)
{
  ExpectNavigationNormalized(
      "at://foo.com/com.example.foo/cursor/3n2tjsaiwrs24",
      "at://foo.com/com.example.foo/cursor/3n2tjsaiwrs24");
  ExpectNavigationNormalized("at://foo.com/com.example.foo/cursor/abc%3D",
                             "at://foo.com/com.example.foo/cursor/abc%3D");

  AtURI uri;
  ASSERT_EQ(
      ParseNavigationAtURI("at://foo.com/com.example.foo/cursor/abc%3D", uri),
      AtURIError::None);
  EXPECT_TRUE(uri.mHasCollection);
  EXPECT_FALSE(uri.mHasRecordKey);
  EXPECT_TRUE(uri.mHasCursor);
  EXPECT_EQ(uri.mCursor, "abc%3D");

  ExpectError("at://foo.com/com.example.foo/cursor/3n2tjsaiwrs24",
              AtURIError::InvalidPath);
}

TEST(AtURI, InvalidCursors)
{
  ExpectError("at://foo.com/com.example.foo/cursor/", AtURIError::InvalidPath);
  AtURI uri;
  EXPECT_EQ(ParseNavigationAtURI("at://foo.com/com.example.foo/cursor/", uri),
            AtURIError::InvalidPath);
  EXPECT_EQ(ParseNavigationAtURI(
                "at://foo.com/com.example.foo/cursor/a&limit=100", uri),
            AtURIError::InvalidCursor);
  EXPECT_EQ(
      ParseNavigationAtURI("at://foo.com/com.example.foo/cursor/abc%2", uri),
      AtURIError::InvalidCursor);
  EXPECT_EQ(
      ParseNavigationAtURI("at://foo.com/com.example.foo/cursor/abc%3d", uri),
      AtURIError::InvalidCursor);
  EXPECT_EQ(
      ParseNavigationAtURI("at://foo.com/com.example.foo/self/cursor/abc", uri),
      AtURIError::InvalidPath);
}

TEST(AtURI, LengthLimit)
{
  std::string tooLong(8193, 'a');
  tooLong.insert(0, "at://");
  ExpectError(tooLong, AtURIError::TooLong);
}

TEST(AtURI, OfficialAtUriSyntaxVectors)
{
  for (auto value : CorpusLines(kAtUriValid)) {
    AtURI uri;
    EXPECT_EQ(ParseAtURI(value, uri), AtURIError::None)
        << "valid vector rejected: " << value;
  }
  for (auto value : CorpusLines(kAtUriInvalid)) {
    AtURI uri;
    EXPECT_NE(ParseAtURI(value, uri), AtURIError::None)
        << "invalid vector accepted: " << value;
  }
}

TEST(AtURI, OfficialPrimitiveSyntaxVectors)
{
  ExpectPrimitiveCorpus(kDidValid, kDidInvalid, IsValidDid);
  ExpectPrimitiveCorpus(kHandleValid, kHandleInvalid, IsValidHandle);

  for (auto value : CorpusLines(kNsidValid)) {
    auto domainEnd = value.rfind('.');
    if (domainEnd != std::string_view::npos && domainEnd > 253) {
      continue;
    }
    EXPECT_TRUE(IsValidNsid(value)) << "valid vector rejected: " << value;
  }
  for (auto value : CorpusLines(kNsidInvalid)) {
    EXPECT_FALSE(IsValidNsid(value)) << "invalid vector accepted: " << value;
  }

  ExpectPrimitiveCorpus(kRecordKeyValid, kRecordKeyInvalid, IsValidRecordKey);
  ExpectPrimitiveCorpus(
      kAtIdentifierValid, kAtIdentifierInvalid, [](std::string_view aValue) {
        return aValue.rfind("did:", 0) == 0 ? IsValidDid(aValue)
                                            : IsValidHandle(aValue);
      });
}
