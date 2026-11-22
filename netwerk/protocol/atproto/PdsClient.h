/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef netwerk_protocol_atproto_PdsClient_h
#define netwerk_protocol_atproto_PdsClient_h

#include <cstdint>
#include <string>
#include <string_view>

namespace mozilla::net::atproto {

enum class PdsUrlError {
  None,
  InvalidEndpoint,
  InvalidRepo,
  InvalidCollection,
  InvalidRecordKey,
  InvalidCursor,
  InvalidCid,
};

const char* PdsUrlErrorName(PdsUrlError aError);

// Build a com.atproto.repo.getRecord request URL from a PDS serviceEndpoint.
// Query values are percent-encoded, so DID colons and record keys with special
// characters cannot break out into additional query parameters.
PdsUrlError BuildGetRecordUrl(std::string_view aEndpoint,
                              std::string_view aRepo,
                              std::string_view aCollection,
                              std::string_view aRecordKey, std::string& aUrl);

// Build a com.atproto.repo.describeRepo request URL for a repository summary.
PdsUrlError BuildDescribeRepoUrl(std::string_view aEndpoint,
                                 std::string_view aRepo, std::string& aUrl);

// Build a com.atproto.repo.listRecords request URL for a collection listing.
// aCursor is an already percent-encoded token from an at:// cursor segment.
PdsUrlError BuildListRecordsUrl(std::string_view aEndpoint,
                                std::string_view aRepo,
                                std::string_view aCollection,
                                std::string_view aCursor, std::string& aUrl,
                                uint32_t aLimit = 50);

// Build a com.atproto.sync.getBlob request URL for a record's blob reference.
PdsUrlError BuildGetBlobUrl(std::string_view aEndpoint, std::string_view aRepo,
                            std::string_view aCid, std::string& aUrl);

// Percent-encode a query value (only RFC 3986 unreserved bytes are kept).
std::string EncodeQueryValue(std::string_view aValue);

}  // namespace mozilla::net::atproto

#endif
