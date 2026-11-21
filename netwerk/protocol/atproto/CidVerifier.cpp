/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "CidVerifier.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

#include "mozilla/crypto_hash_sha2.h"

namespace mozilla::net::atproto {

namespace {

constexpr uint8_t kCidVersion1 = 0x01;
constexpr uint8_t kCodecDagCbor = 0x71;
constexpr uint8_t kMultihashSha256 = 0x12;
constexpr uint8_t kDigestLength = 32;
constexpr size_t kCidV1Sha256Size = 4 + kDigestLength;
constexpr size_t kCidV1Sha256Base32Digits = (kCidV1Sha256Size * 8 + 4) / 5;

int Base32Value(char aChar) {
  if (aChar >= 'a' && aChar <= 'z') {
    return aChar - 'a';
  }
  if (aChar >= '2' && aChar <= '7') {
    return 26 + (aChar - '2');
  }
  return -1;
}

bool DecodeCid(std::string_view aCid, std::vector<uint8_t>& aOut) {
  aOut.clear();
  if (aCid.empty() || aCid[0] != 'b' ||
      aCid.size() != 1 + kCidV1Sha256Base32Digits) {
    return false;
  }

  uint32_t bits = 0;
  int bitsLeft = 0;
  for (size_t i = 1; i < aCid.size(); ++i) {
    int value = Base32Value(aCid[i]);
    if (value < 0) {
      return false;
    }
    bits = (bits << 5) | static_cast<uint32_t>(value);
    bitsLeft += 5;
    if (bitsLeft >= 8) {
      bitsLeft -= 8;
      aOut.push_back(static_cast<uint8_t>(bits >> bitsLeft));
      bits &= (1u << bitsLeft) - 1;
    }
  }
  if (bitsLeft > 0 && bits != 0) {
    return false;
  }

  return aOut.size() == kCidV1Sha256Size && aOut[0] == kCidVersion1 &&
         aOut[1] == kCodecDagCbor && aOut[2] == kMultihashSha256 &&
         aOut[3] == kDigestLength;
}

void AppendHead(std::vector<uint8_t>& aOut, uint8_t aMajor, uint64_t aValue) {
  uint8_t major = aMajor << 5;
  if (aValue < 24) {
    aOut.push_back(major | static_cast<uint8_t>(aValue));
  } else if (aValue <= 0xff) {
    aOut.push_back(major | 24);
    aOut.push_back(static_cast<uint8_t>(aValue));
  } else if (aValue <= 0xffff) {
    aOut.push_back(major | 25);
    aOut.push_back(static_cast<uint8_t>(aValue >> 8));
    aOut.push_back(static_cast<uint8_t>(aValue));
  } else if (aValue <= 0xffffffff) {
    aOut.push_back(major | 26);
    for (int shift = 24; shift >= 0; shift -= 8) {
      aOut.push_back(static_cast<uint8_t>(aValue >> shift));
    }
  } else {
    aOut.push_back(major | 27);
    for (int shift = 56; shift >= 0; shift -= 8) {
      aOut.push_back(static_cast<uint8_t>(aValue >> shift));
    }
  }
}

void AppendText(std::vector<uint8_t>& aOut, const std::string& aText) {
  AppendHead(aOut, 3, aText.size());
  aOut.insert(aOut.end(), aText.begin(), aText.end());
}

bool EncodeValue(const Json::Value& aValue, std::vector<uint8_t>& aOut) {
  switch (aValue.type()) {
    case Json::nullValue:
      aOut.push_back(0xf6);
      return true;
    case Json::booleanValue:
      aOut.push_back(aValue.asBool() ? 0xf5 : 0xf4);
      return true;
    case Json::intValue: {
      int64_t number = aValue.asInt64();
      if (number >= 0) {
        AppendHead(aOut, 0, static_cast<uint64_t>(number));
      } else {
        AppendHead(aOut, 1, static_cast<uint64_t>(-1 - number));
      }
      return true;
    }
    case Json::uintValue:
      AppendHead(aOut, 0, aValue.asUInt64());
      return true;
    case Json::stringValue:
      AppendText(aOut, aValue.asString());
      return true;
    case Json::arrayValue:
      AppendHead(aOut, 4, aValue.size());
      for (const auto& item : aValue) {
        if (!EncodeValue(item, aOut)) {
          return false;
        }
      }
      return true;
    case Json::objectValue: {
      std::vector<std::pair<std::vector<uint8_t>, const Json::Value*>> entries;
      entries.reserve(aValue.size());
      for (const std::string& name : aValue.getMemberNames()) {
        std::vector<uint8_t> key;
        AppendText(key, name);
        entries.emplace_back(std::move(key), &aValue[name]);
      }
      // DAG-CBOR orders map keys by encoded length, then bytewise.
      std::sort(entries.begin(), entries.end(),
                [](const auto& aLeft, const auto& aRight) {
                  if (aLeft.first.size() != aRight.first.size()) {
                    return aLeft.first.size() < aRight.first.size();
                  }
                  return aLeft.first < aRight.first;
                });
      AppendHead(aOut, 5, entries.size());
      for (const auto& entry : entries) {
        aOut.insert(aOut.end(), entry.first.begin(), entry.first.end());
        if (!EncodeValue(*entry.second, aOut)) {
          return false;
        }
      }
      return true;
    }
    case Json::realValue:
      return false;
  }
  return false;
}

}  // namespace

const char* CidStatusName(CidStatus aStatus) {
  switch (aStatus) {
    case CidStatus::Absent:
      return "Absent";
    case CidStatus::Verified:
      return "Verified";
    case CidStatus::Mismatch:
      return "Mismatch";
    case CidStatus::InvalidCid:
      return "InvalidCid";
    case CidStatus::UnsupportedValue:
      return "UnsupportedValue";
  }
  return "Unknown";
}

CidStatus VerifyRecordCid(std::string_view aCid, const Json::Value& aValue) {
  if (aCid.empty()) {
    return CidStatus::Absent;
  }

  std::vector<uint8_t> cidBytes;
  if (!DecodeCid(aCid, cidBytes)) {
    return CidStatus::InvalidCid;
  }

  std::vector<uint8_t> cbor;
  if (!EncodeValue(aValue, cbor) || cbor.empty()) {
    return CidStatus::UnsupportedValue;
  }

  std::array<uint8_t, 32> digest;
  crypto_hash_sha256(cbor.data(), cbor.size(), digest.data());
  for (size_t i = 0; i < kDigestLength; ++i) {
    if (digest[i] != cidBytes[4 + i]) {
      return CidStatus::Mismatch;
    }
  }
  return CidStatus::Verified;
}

}  // namespace mozilla::net::atproto
