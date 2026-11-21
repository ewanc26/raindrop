/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "AtURI.h"
#include "gtest/gtest.h"

using namespace mozilla::net::atproto;

namespace {

void ExpectNormalized(std::string_view aInput, std::string_view aExpected) {
  AtURIError error = AtURIError::None;
  EXPECT_EQ(NormalizeAtURI(aInput, error), aExpected) << "for input " << aInput;
  EXPECT_EQ(error, AtURIError::None) << AtURIErrorName(error);
}

void ExpectError(std::string_view aInput, AtURIError aExpected) {
  AtURIError error = AtURIError::None;
  AtURI uri;
  error = ParseAtURI(aInput, uri);
  EXPECT_EQ(error, aExpected)
      << "for input " << aInput << ", got " << AtURIErrorName(error);
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
  ExpectNormalized("AT://Jay.Bsky.Social/Com.EXAMPLE.FooBar/self",
                   "at://jay.bsky.social/com.example.FooBar/self");
  ExpectNormalized("at://did:plc:x/app.bsky.feed.post/%73elf",
                   "at://did:plc:x/app.bsky.feed.post/self");
  ExpectNormalized("at://did:plc:x/%61pp.bsky.feed.post/self",
                   "at://did:plc:x/app.bsky.feed.post/self");
  ExpectNormalized("at://foo.com/com.example.foo/123#/%ff",
                   "at://foo.com/com.example.foo/123#/%FF");
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
  ExpectNormalized("at://foo.com/com.example.foo/~1.2-3_",
                   "at://foo.com/com.example.foo/~1.2-3_");
}

TEST(AtURI, CursorSegments)
{
  ExpectNormalized("at://foo.com/com.example.foo/cursor/3n2tjsaiwrs24",
                   "at://foo.com/com.example.foo/cursor/3n2tjsaiwrs24");
  ExpectNormalized("at://foo.com/com.example.foo/cursor/abc%3D",
                   "at://foo.com/com.example.foo/cursor/abc%3D");

  AtURI uri;
  ASSERT_EQ(ParseAtURI("at://foo.com/com.example.foo/cursor/abc%3D", uri),
            AtURIError::None);
  EXPECT_TRUE(uri.mHasCollection);
  EXPECT_FALSE(uri.mHasRecordKey);
  EXPECT_TRUE(uri.mHasCursor);
  EXPECT_EQ(uri.mCursor, "abc%3D");
}

TEST(AtURI, InvalidCursors)
{
  ExpectError("at://foo.com/com.example.foo/cursor/", AtURIError::InvalidPath);
  ExpectError("at://foo.com/com.example.foo/cursor/a&limit=100",
              AtURIError::InvalidCursor);
  ExpectError("at://foo.com/com.example.foo/cursor/abc%2",
              AtURIError::InvalidCursor);
  ExpectError("at://foo.com/com.example.foo/cursor/abc%3d",
              AtURIError::InvalidCursor);
  ExpectError("at://foo.com/com.example.foo/self/cursor/abc",
              AtURIError::InvalidPath);
}

TEST(AtURI, LengthLimit)
{
  std::string tooLong(8193, 'a');
  tooLong.insert(0, "at://");
  ExpectError(tooLong, AtURIError::TooLong);
}
