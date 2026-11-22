/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef netwerk_protocol_atproto_RecordView_h
#define netwerk_protocol_atproto_RecordView_h

#include <string>
#include <string_view>
#include <vector>

#include "AtDocumentPolicy.h"
#include "CidVerifier.h"

namespace mozilla::net::atproto {

enum class RecordViewError {
  None,
  TooLong,
  InvalidJson,
  InvalidEnvelope,
  MissingValue,
};

const char* RecordViewErrorName(RecordViewError aError);

constexpr size_t kMaxRecordResponseLength = 4 * 1024 * 1024;

struct RecordField {
  std::string mName;
  std::string mValue;
};

struct RecordViewData {
  std::string mCid;
  CidStatus mCidStatus = CidStatus::Absent;
  std::string mLexicon;
  std::string mRawJson;
  std::string mValueJson;
  std::vector<RecordField> mFields;
};

RecordViewError ParseRecordResponse(std::string_view aJson,
                                    RecordViewData& aOut);

struct RecordViewProvenance {
  std::string_view mDid;
  std::string_view mHandle;
  std::string_view mPds;
  std::string_view mCollection;
  std::string_view mRecordKey;
};

struct RecordViewOptions {
  bool mShowProvenance = true;
  bool mShowRawJson = true;
  bool mShowImages = false;
};

std::string RenderRecordHtml(const RecordViewData& aData,
                             const RecordViewProvenance& aProvenance,
                             const RecordViewOptions& aOptions = {},
                             const ViewStyleOptions& aStyle = {});

}  // namespace mozilla::net::atproto

#endif  // netwerk_protocol_atproto_RecordView_h
