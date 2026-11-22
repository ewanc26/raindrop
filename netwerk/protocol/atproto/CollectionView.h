/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef netwerk_protocol_atproto_CollectionView_h
#define netwerk_protocol_atproto_CollectionView_h

#include <string>
#include <string_view>
#include <vector>

#include "AtDocumentPolicy.h"

namespace mozilla::net::atproto {

enum class CollectionViewError {
  None,
  TooLong,
  InvalidJson,
  MissingRecords,
  InvalidRecord,
};

const char* CollectionViewErrorName(CollectionViewError aError);

constexpr size_t kMaxCollectionResponseLength = 4 * 1024 * 1024;

struct CollectionViewRecord {
  std::string mRecordKey;
  std::string mCid;
  std::string mSummaryType;
  std::string mSummaryText;
  std::string mSummaryWhen;
};

struct CollectionViewData {
  std::vector<CollectionViewRecord> mRecords;
  std::string mCursor;
};

struct CollectionViewProvenance {
  std::string mDid;
  std::string mHandle;
  std::string mPds;
  std::string mCollection;
  std::string mCursor;
};

CollectionViewError ParseCollectionResponse(std::string_view aJson,
                                            CollectionViewData& aOut);

std::string RenderCollectionHtml(const CollectionViewData& aData,
                                 const CollectionViewProvenance& aProvenance,
                                 const ViewStyleOptions& aStyle = {});

}  // namespace mozilla::net::atproto

#endif  // netwerk_protocol_atproto_CollectionView_h
