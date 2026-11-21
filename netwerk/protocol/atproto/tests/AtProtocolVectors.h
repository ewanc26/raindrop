/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef netwerk_protocol_atproto_tests_AtProtocolVectors_h
#define netwerk_protocol_atproto_tests_AtProtocolVectors_h

#include <string_view>

// Copied from https://github.com/bluesky-social/atproto-interop-tests
// (CC0 1.0 Universal). The NSID valid corpus contains one vector that
// exceeds the spec limit for the domain authority; it is skipped.

namespace mozilla::net::atproto {

inline constexpr std::string_view kAtUriValid = R"VECTOR(
# enforces spec basics
at://did:plc:asdf123
at://user.bsky.social
at://did:plc:asdf123/com.atproto.feed.post
at://did:plc:asdf123/com.atproto.feed.post/record

# very long: 'at://did:plc:asdf123/com.atproto.feed.post/' + 'o'.repeat(512)
at://did:plc:asdf123/com.atproto.feed.post/oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo

# enforces no trailing slashes
at://did:plc:asdf123
at://user.bsky.social
at://did:plc:asdf123/com.atproto.feed.post
at://did:plc:asdf123/com.atproto.feed.post/record

# enforces strict paths
at://did:plc:asdf123/com.atproto.feed.post/asdf123

# is very permissive about record keys
at://did:plc:asdf123/com.atproto.feed.post/asdf123
at://did:plc:asdf123/com.atproto.feed.post/a

at://did:plc:asdf123/com.atproto.feed.post/asdf-123
at://did:abc:123
at://did:abc:123/io.nsid.someFunc/record-key

at://did:abc:123/io.nsid.someFunc/self.
at://did:abc:123/io.nsid.someFunc/lang:
at://did:abc:123/io.nsid.someFunc/:
at://did:abc:123/io.nsid.someFunc/-
at://did:abc:123/io.nsid.someFunc/_
at://did:abc:123/io.nsid.someFunc/~
at://did:abc:123/io.nsid.someFunc/...
at://did:plc:asdf123/com.atproto.feed.postV2
)VECTOR";

inline constexpr std::string_view kAtUriInvalid = R"VECTOR(
# enforces spec basics
a://did:plc:asdf123
at//did:plc:asdf123
at:/a/did:plc:asdf123
at:/did:plc:asdf123
AT://did:plc:asdf123
http://did:plc:asdf123
://did:plc:asdf123
at:did:plc:asdf123
at:/did:plc:asdf123
at:///did:plc:asdf123
at://:/did:plc:asdf123
at:/ /did:plc:asdf123
at://did:plc:asdf123 
at://did:plc:asdf123/ 
 at://did:plc:asdf123
at://did:plc:asdf123/com.atproto.feed.post 
at://did:plc:asdf123/com.atproto.feed.post# 
at://did:plc:asdf123/com.atproto.feed.post#/ 
at://did:plc:asdf123/com.atproto.feed.post#/frag 
at://did:plc:asdf123/com.atproto.feed.post#fr ag
//did:plc:asdf123
at://name
at://name.0
at://diD:plc:asdf123
at://did:plc:asdf123/com.atproto.feed.p@st
at://did:plc:asdf123/com.atproto.feed.p$st
at://did:plc:asdf123/com.atproto.feed.p%st
at://did:plc:asdf123/com.atproto.feed.p&st
at://did:plc:asdf123/com.atproto.feed.p()t
at://did:plc:asdf123/com.atproto.feed_post
at://did:plc:asdf123/-com.atproto.feed.post
at://did:plc:asdf@123/com.atproto.feed.post
at://DID:plc:asdf123
at://user.bsky.123
at://bsky
at://did:plc:
at://did:plc:
at://frag

# too long: 'at://did:plc:asdf123/com.atproto.feed.post/' + 'o'.repeat(8200)
at://did:plc:asdf123/com.atproto.feed.post/oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo

# has specified behavior on edge cases
at://user.bsky.social//
at://user.bsky.social//com.atproto.feed.post
at://user.bsky.social/com.atproto.feed.post//
at://did:plc:asdf123/com.atproto.feed.post/asdf123/more/more',
at://did:plc:asdf123/short/stuff
at://did:plc:asdf123/12345

# enforces no trailing slashes
at://did:plc:asdf123/
at://user.bsky.social/
at://did:plc:asdf123/com.atproto.feed.post/
at://did:plc:asdf123/com.atproto.feed.post/record/
at://did:plc:asdf123/com.atproto.feed.post/record/#/frag

# enforces strict paths
at://did:plc:asdf123/com.atproto.feed.post/asdf123/asdf

# is very permissive about fragments
at://did:plc:asdf123#
at://did:plc:asdf123##
#at://did:plc:asdf123
at://did:plc:asdf123#/asdf#/asdf

# new less permissive about record keys for Lexicon use (with recordkey more specified)
at://did:plc:asdf123/com.atproto.feed.post/%23
at://did:plc:asdf123/com.atproto.feed.post/$@!*)(:,;~.sdf123
at://did:plc:asdf123/com.atproto.feed.post/~'sdf123")
at://did:plc:asdf123/com.atproto.feed.post/$
at://did:plc:asdf123/com.atproto.feed.post/@
at://did:plc:asdf123/com.atproto.feed.post/!
at://did:plc:asdf123/com.atproto.feed.post/*
at://did:plc:asdf123/com.atproto.feed.post/(
at://did:plc:asdf123/com.atproto.feed.post/,
at://did:plc:asdf123/com.atproto.feed.post/;
at://did:plc:asdf123/com.atproto.feed.post/abc%30123
at://did:plc:asdf123/com.atproto.feed.post/%30
at://did:plc:asdf123/com.atproto.feed.post/%3
at://did:plc:asdf123/com.atproto.feed.post/%
at://did:plc:asdf123/com.atproto.feed.post/%zz
at://did:plc:asdf123/com.atproto.feed.post/%%%

# disallow dot / double-dot
at://did:plc:asdf123/com.atproto.feed.post/.
at://did:plc:asdf123/com.atproto.feed.post/..
)VECTOR";

inline constexpr std::string_view kDidValid = R"VECTOR(did:method:val
did:method:VAL
did:method:val123
did:method:123
did:method:val-two
did:method:val_two
did:method:val.two
did:method:val:two
did:method:val%BB
did:method:vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
did:m:v
did:method::::val
did:method:-
did:method:-:_:.:%ab
did:method:.
did:method:_
did:method::.

# allows some real DID values
did:onion:2gzyxa5ihm7nsggfxnu52rck2vv4rvmdlkiu3zzui5du4xyclen53wid
did:example:123456789abcdefghi
did:plc:7iza6de2dwap2sbkpav7c6c6
did:web:example.com
did:web:localhost%3A1234
did:key:zQ3shZc2QzApp2oymGvQbzP8eKheVshBHbU4ZYjeXqwSKEn6N
did:ethr:0xb9c5714089478a327f09197987f16f9e5d936e8a
)VECTOR";

inline constexpr std::string_view kDidInvalid = R"VECTOR(did
didmethodval
method:did:val
did:method:
didmethod:val
did:methodval)
:did:method:val
did.method.val
did:method:val:
did:method:val%
DID:method:val
did:METHOD:val
did:m123:val
did:method:val/two
did:method:val?two
did:method:val#two
did:method:val%
did:method:vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
)VECTOR";

inline constexpr std::string_view kHandleValid = R"VECTOR(# allows valid handles
A.ISI.EDU
XX.LCS.MIT.EDU
SRI-NIC.ARPA
john.test
jan.test
a234567890123456789.test
john2.test
john-john.test
john.bsky.app
jo.hn
a.co
a.org
joh.n
j0.h0
jaymome-johnber123456.test
jay.mome-johnber123456.test
john.test.bsky.app

# max over all handle: 'shoooort' + '.loooooooooooooooooooooooooong'.repeat(8) + '.test'
shoooort.loooooooooooooooooooooooooong.loooooooooooooooooooooooooong.loooooooooooooooooooooooooong.loooooooooooooooooooooooooong.loooooooooooooooooooooooooong.loooooooooooooooooooooooooong.loooooooooooooooooooooooooong.loooooooooooooooooooooooooong.test

# max segment: 'short.' + 'o'.repeat(63) + '.test'
short.ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo.test

# NOTE: this probably isn't ever going to be a real domain, but my read of the RFC is that it would be possible
john.t

# allows .local and .arpa handles (proto-level)
laptop.local
laptop.arpa

# allows punycode handles
# 💩.test
xn--ls8h.test
# bücher.tld
xn--bcher-kva.tld
xn--3jk.com
xn--w3d.com
xn--vqb.com
xn--ppd.com
xn--cs9a.com
xn--8r9a.com
xn--cfd.com
xn--5jk.com
xn--2lb.com

# allows onion (Tor) handles
expyuzz4wqqyqhjn.onion
friend.expyuzz4wqqyqhjn.onion
g2zyxa5ihm7nsggfxnu52rck2vv4rvmdlkiu3zzui5du4xyclen53wid.onion
friend.g2zyxa5ihm7nsggfxnu52rck2vv4rvmdlkiu3zzui5du4xyclen53wid.onion
friend.g2zyxa5ihm7nsggfxnu52rck2vv4rvmdlkiu3zzui5du4xyclen53wid.onion
2gzyxa5ihm7nsggfxnu52rck2vv4rvmdlkiu3zzui5du4xyclen53wid.onion
friend.2gzyxa5ihm7nsggfxnu52rck2vv4rvmdlkiu3zzui5du4xyclen53wid.onion

# correctly validates corner cases (modern vs. old RFCs)
12345.test
8.cn
4chan.org
4chan.o-g
blah.4chan.org
thing.a01
120.0.0.1.com
0john.test
9sta--ck.com
99stack.com
0ohn.test
john.t--t
thing.0aa.thing

# examples from stackoverflow   
stack.com
sta-ck.com
sta---ck.com
sta--ck9.com
stack99.com
sta99ck.com
google.com.uk
google.co.in
google.com
maselkowski.pl
m.maselkowski.pl
xn--masekowski-d0b.pl
xn--fiqa61au8b7zsevnm8ak20mc4a87e.xn--fiqs8s
xn--stackoverflow.com
stackoverflow.xn--com
stackoverflow.co.uk
xn--masekowski-d0b.pl
xn--fiqa61au8b7zsevnm8ak20mc4a87e.xn--fiqs8s
)VECTOR";

inline constexpr std::string_view kHandleInvalid = R"VECTOR(# throws on invalid handles
did:thing.test
did:thing
john-.test
john.0
john.-
xn--bcher-.tld
john..test
jo_hn.test
-john.test
.john.test
jo!hn.test
jo%hn.test
jo&hn.test
jo@hn.test
jo*hn.test
jo|hn.test
jo:hn.test
jo/hn.test
john💩.test
bücher.test
john .test
john.test.
john
john.
.john
john.test.
.john.test
 john.test
john.test 
joh-.test
john.-est
john.tes-

# max over all handle: 'shoooort' + '.loooooooooooooooooooooooooong'.repeat(9) + '.test'
shoooort.loooooooooooooooooooooooooong.loooooooooooooooooooooooooong.loooooooooooooooooooooooooong.loooooooooooooooooooooooooong.loooooooooooooooooooooooooong.loooooooooooooooooooooooooong.loooooooooooooooooooooooooong.loooooooooooooooooooooooooong.loooooooooooooooooooooooooong.test

# max segment: 'short.' + 'o'.repeat(64) + '.test'
short.oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo.test

# throws on "dotless" TLD handles
org
ai
gg
io

# correctly validates corner cases (modern vs. old RFCs)
cn.8
thing.0aa
thing.0aa

# does not allow IP addresses as handles
127.0.0.1
192.168.0.142
fe80::7325:8a97:c100:94b
2600:3c03::f03c:9100:feb0:af1f

# examples from stackoverflow   
-notvalid.at-all
-thing.com
www.masełkowski.pl.com
)VECTOR";

inline constexpr std::string_view kNsidValid = R"VECTOR(# length checks
com.ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo.foo
com.example.ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
com.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.foo

# valid examples
com.example.fooBar
com.example.fooBarV2
net.users.bob.ping
a.b.c
m.xn--masekowski-d0b.pl
one.two.three
one.two.three.four-and.FiVe
one.2.three
a-0.b-1.c
a0.b1.cc
cn.8.lex.stuff
test.12345.record
a01.thing.record
a.0.c
xn--fiqs8s.xn--fiqa61au8b7zsevnm8ak20mc4a87e.record.two
a0.b1.c3
com.example.f00

# allows onion (Tor) NSIDs
onion.expyuzz4wqqyqhjn.spec.getThing
onion.g2zyxa5ihm7nsggfxnu52rck2vv4rvmdlkiu3zzui5du4xyclen53wid.lex.deleteThing

# allows starting-with-numeric segments (same as domains)
org.4chan.lex.getThing
cn.8.lex.stuff
onion.2gzyxa5ihm7nsggfxnu52rck2vv4rvmdlkiu3zzui5du4xyclen53wid.lex.deleteThing
)VECTOR";

inline constexpr std::string_view kNsidInvalid = R"VECTOR(# length checks
com.oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo.foo
com.example.oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
com.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.middle.foo

# invalid examples
com.example.foo.*
com.example.foo.blah*
com.example.foo.*blah
com.exa💩ple.thing
a-0.b-1.c-3
a-0.b-1.c-o
1.0.0.127.record
0two.example.foo
example.com
com.example
a.
.one.two.three
one.two.three 
one.two..three
one .two.three
 one.two.three
com.exa💩ple.thing
com.atproto.feed.p@st
com.atproto.feed.p_st
com.atproto.feed.p*st
com.atproto.feed.po#t
com.atproto.feed.p!ot
com.example-.foo
com.example.fooBar.2
)VECTOR";

inline constexpr std::string_view kRecordKeyValid = R"VECTOR(# specs
self
example.com
~1.2-3_
dHJ1ZQ
_
literal:self
pre:fix

# more corner-cases
:
-
_
~
...
self.
lang:
:lang

# very long: 'o'.repeat(512)
oooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
)VECTOR";

inline constexpr std::string_view kRecordKeyInvalid = R"VECTOR(# specs
alpha/beta
.
..
#extra
@handle
any space
any+space
number[3]
number(3)
"quote"
dHJ1ZQ==

# too long: 'o'.repeat(513)
ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
)VECTOR";

inline constexpr std::string_view kAtIdentifierValid = R"VECTOR(
# allows valid handles
XX.LCS.MIT.EDU
john.test
jan.test
a234567890123456789.test
john2.test
john-john.test

# allows valid DIDs
did:method:val
did:method:VAL
did:method:val123
did:method:123
did:method:val-two
)VECTOR";

inline constexpr std::string_view kAtIdentifierInvalid = R"VECTOR(
# invalid handles
did:thing.test
did:thing
john-.test
john.0
john.-
xn--bcher-.tld
john..test
jo_hn.test

# invalid DIDs
did
didmethodval
method:did:val
did:method:
didmethod:val
did:methodval)
:did:method:val
did:method:val:
did:method:val%
DID:method:val

# other invalid stuff
email@example.com
@handle@example.com
@handle
blah
)VECTOR";

}  // namespace mozilla::net::atproto

#endif