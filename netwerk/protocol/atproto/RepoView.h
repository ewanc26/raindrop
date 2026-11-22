/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef netwerk_protocol_atproto_RepoView_h
#define netwerk_protocol_atproto_RepoView_h

#include <string>
#include <string_view>
#include <vector>

#include "AtDocumentPolicy.h"

namespace mozilla::net::atproto {

enum class RepoViewError {
  None,
  TooLong,
  InvalidJson,
  MissingDid,
};

const char* RepoViewErrorName(RepoViewError aError);

constexpr size_t kMaxRepoResponseLength = 4 * 1024 * 1024;

struct RepoViewData {
  std::string mDid;
  std::string mHandle;
  std::vector<std::string> mCollections;
};

struct RepoViewProvenance {
  std::string mDid;
  std::string mHandle;
  std::string mPds;
};

RepoViewError ParseRepoResponse(std::string_view aJson, RepoViewData& aOut);

std::string RenderRepoHtml(const RepoViewData& aData,
                           const RepoViewProvenance& aProvenance,
                           const ViewStyleOptions& aStyle = {});

}  // namespace mozilla::net::atproto

#endif  // netwerk_protocol_atproto_RepoView_h
