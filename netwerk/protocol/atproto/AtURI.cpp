/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "AtURI.h"

#include <vector>

namespace mozilla::net::atproto {

namespace {

constexpr size_t kMaxURILength = 8192;
constexpr size_t kMaxDIDLength = 2048;
constexpr size_t kMaxHandleLength = 253;
constexpr size_t kMaxNsidLength = 317;
constexpr size_t kMaxNsidDomainLength = 253;
constexpr size_t kMaxLabelLength = 63;
constexpr size_t kMaxRecordKeyLength = 512;

bool IsAlpha(char aChar) {
  return (aChar >= 'a' && aChar <= 'z') || (aChar >= 'A' && aChar <= 'Z');
}

bool IsDigit(char aChar) { return aChar >= '0' && aChar <= '9'; }

bool IsAlnum(char aChar) { return IsAlpha(aChar) || IsDigit(aChar); }

bool IsHexDigit(char aChar) {
  return IsDigit(aChar) || (aChar >= 'a' && aChar <= 'f') ||
         (aChar >= 'A' && aChar <= 'F');
}

bool IsUnreserved(char aChar) {
  return IsAlnum(aChar) || aChar == '.' || aChar == '-' || aChar == '_' ||
         aChar == '~';
}

char ToUpperAscii(char aChar) {
  return aChar >= 'a' && aChar <= 'z' ? aChar - 'a' + 'A' : aChar;
}

char ToLowerAscii(char aChar) {
  return aChar >= 'A' && aChar <= 'Z' ? aChar - 'A' + 'a' : aChar;
}

bool StartsWith(std::string_view aValue, std::string_view aPrefix) {
  return aValue.substr(0, aPrefix.size()) == aPrefix;
}

bool StartsWithIgnoreCase(std::string_view aValue, std::string_view aPrefix) {
  if (aValue.size() < aPrefix.size()) {
    return false;
  }
  for (size_t i = 0; i < aPrefix.size(); ++i) {
    if (ToLowerAscii(aValue[i]) != ToLowerAscii(aPrefix[i])) {
      return false;
    }
  }
  return true;
}

std::string LowerAscii(std::string_view aValue) {
  std::string result(aValue);
  for (char& c : result) {
    c = ToLowerAscii(c);
  }
  return result;
}

std::vector<std::string_view> Split(std::string_view aValue, char aSeparator) {
  std::vector<std::string_view> parts;
  size_t start = 0;
  while (true) {
    size_t end = aValue.find(aSeparator, start);
    if (end == std::string_view::npos) {
      parts.push_back(aValue.substr(start));
      return parts;
    }
    parts.push_back(aValue.substr(start, end - start));
    start = end + 1;
  }
}

bool DecodeUnreserved(std::string_view aInput, std::string& aOutput) {
  aOutput.clear();
  for (size_t i = 0; i < aInput.size(); ++i) {
    char c = aInput[i];
    if (c != '%') {
      aOutput.push_back(c);
      continue;
    }
    if (i + 2 >= aInput.size() || !IsHexDigit(aInput[i + 1]) ||
        !IsHexDigit(aInput[i + 2])) {
      return false;
    }
    char decoded = static_cast<char>((IsDigit(aInput[i + 1])
                                          ? aInput[i + 1] - '0'
                                          : ToUpperAscii(aInput[i + 1]) - 'A' +
                                                10) *
                                         16 +
                                     (IsDigit(aInput[i + 2])
                                          ? aInput[i + 2] - '0'
                                          : ToUpperAscii(aInput[i + 2]) - 'A' +
                                                10));
    if (IsUnreserved(decoded)) {
      aOutput.push_back(decoded);
    } else {
      aOutput.push_back('%');
      aOutput.push_back(ToUpperAscii(aInput[i + 1]));
      aOutput.push_back(ToUpperAscii(aInput[i + 2]));
    }
    i += 2;
  }
  return true;
}

bool HasUppercaseHexTriplets(std::string_view aValue) {
  for (size_t i = 0; i + 2 < aValue.size(); ++i) {
    if (aValue[i] == '%') {
      if (!IsHexDigit(aValue[i + 1]) || !IsHexDigit(aValue[i + 2])) {
        return false;
      }
      i += 2;
    }
  }
  return true;
}

std::string NormalizeDid(std::string_view aDid) {
  std::string result(aDid);
  for (size_t i = 0; i + 2 < result.size(); ++i) {
    if (result[i] == '%') {
      result[i + 1] = ToUpperAscii(result[i + 1]);
      result[i + 2] = ToUpperAscii(result[i + 2]);
      i += 2;
    }
  }
  return result;
}

std::string NormalizeNsid(std::string_view aNsid) {
  auto parts = Split(aNsid, '.');
  std::string result;
  for (size_t i = 0; i < parts.size(); ++i) {
    if (i) {
      result.push_back('.');
    }
    if (i + 1 < parts.size()) {
      result += LowerAscii(parts[i]);
    } else {
      result += parts[i];
    }
  }
  return result;
}

bool IsValidLabel(std::string_view aLabel) {
  if (aLabel.empty() || aLabel.size() > kMaxLabelLength) {
    return false;
  }
  if (aLabel.front() == '-' || aLabel.back() == '-') {
    return false;
  }
  for (char c : aLabel) {
    if (!IsAlnum(c) && c != '-') {
      return false;
    }
  }
  return true;
}

}  // namespace

const char* AtURIErrorName(AtURIError aError) {
  switch (aError) {
    case AtURIError::None:
      return "None";
    case AtURIError::TooLong:
      return "TooLong";
    case AtURIError::InvalidScheme:
      return "InvalidScheme";
    case AtURIError::EmptyAuthority:
      return "EmptyAuthority";
    case AtURIError::InvalidAuthority:
      return "InvalidAuthority";
    case AtURIError::InvalidCollection:
      return "InvalidCollection";
    case AtURIError::InvalidRecordKey:
      return "InvalidRecordKey";
    case AtURIError::InvalidPath:
      return "InvalidPath";
  }
  return "Unknown";
}

bool IsValidHandle(std::string_view aHandle) {
  if (aHandle.empty() || aHandle.size() > kMaxHandleLength) {
    return false;
  }
  auto labels = Split(aHandle, '.');
  if (labels.size() < 2) {
    return false;
  }
  for (auto label : labels) {
    if (!IsValidLabel(label)) {
      return false;
    }
  }
  return IsAlpha(labels.back().front());
}

bool IsValidDid(std::string_view aDid) {
  if (aDid.size() > kMaxDIDLength || !StartsWith(aDid, "did:")) {
    return false;
  }
  auto remainder = aDid.substr(4);
  auto separator = remainder.find(':');
  if (separator == std::string_view::npos || separator == 0 ||
      separator + 1 == remainder.size()) {
    return false;
  }
  auto method = remainder.substr(0, separator);
  auto identifier = remainder.substr(separator + 1);
  for (char c : method) {
    if (c < 'a' || c > 'z') {
      return false;
    }
  }
  char last = identifier.back();
  if (!IsAlnum(last) && last != '.' && last != '_' && last != '-') {
    return false;
  }
  for (size_t i = 0; i < identifier.size(); ++i) {
    char c = identifier[i];
    if (c == '%') {
      if (i + 2 >= identifier.size() || !IsHexDigit(identifier[i + 1]) ||
          !IsHexDigit(identifier[i + 2])) {
        return false;
      }
      i += 2;
      continue;
    }
    if (!IsAlnum(c) && c != '.' && c != '_' && c != ':' && c != '-') {
      return false;
    }
  }
  return true;
}

bool IsValidNsid(std::string_view aNsid) {
  if (aNsid.size() < 5 || aNsid.size() > kMaxNsidLength) {
    return false;
  }
  auto parts = Split(aNsid, '.');
  if (parts.size() < 3) {
    return false;
  }
  size_t domainLength = aNsid.size() - parts.back().size() - 1;
  if (domainLength > kMaxNsidDomainLength) {
    return false;
  }
  for (size_t i = 0; i + 1 < parts.size(); ++i) {
    if (!IsValidLabel(parts[i])) {
      return false;
    }
  }
  if (!IsAlpha(parts.front().front())) {
    return false;
  }
  auto name = parts.back();
  if (name.empty() || name.size() > kMaxLabelLength || !IsAlpha(name.front())) {
    return false;
  }
  for (char c : name) {
    if (!IsAlnum(c)) {
      return false;
    }
  }
  return true;
}

bool IsValidRecordKey(std::string_view aRecordKey) {
  if (aRecordKey.empty() || aRecordKey.size() > kMaxRecordKeyLength) {
    return false;
  }
  if (aRecordKey == "." || aRecordKey == "..") {
    return false;
  }
  for (char c : aRecordKey) {
    if (!IsAlnum(c) && c != '.' && c != '_' && c != ':' && c != '~' &&
        c != '-') {
      return false;
    }
  }
  return true;
}

AtURIError ParseAtURI(std::string_view aSpec, AtURI& aResult) {
  aResult = AtURI{};
  if (aSpec.size() > kMaxURILength) {
    return AtURIError::TooLong;
  }
  if (!StartsWithIgnoreCase(aSpec, "at://")) {
    return AtURIError::InvalidScheme;
  }

  std::string_view rest = aSpec.substr(5);
  if (rest.find('?') != std::string_view::npos) {
    return AtURIError::InvalidPath;
  }

  std::string_view fragment;
  auto hash = rest.find('#');
  if (hash != std::string_view::npos) {
    fragment = rest.substr(hash + 1);
    rest = rest.substr(0, hash);
  }

  auto slash = rest.find('/');
  if (slash != std::string_view::npos && slash + 1 == rest.size()) {
    return AtURIError::InvalidPath;
  }
  auto authority = slash == std::string_view::npos ? rest : rest.substr(0, slash);
  auto path = slash == std::string_view::npos ? std::string_view{}
                                              : rest.substr(slash + 1);
  if (authority.empty()) {
    return AtURIError::EmptyAuthority;
  }

  if (StartsWith(authority, "did:")) {
    if (!IsValidDid(authority)) {
      return AtURIError::InvalidAuthority;
    }
    aResult.mAuthority = NormalizeDid(authority);
  } else {
    if (!IsValidHandle(authority)) {
      return AtURIError::InvalidAuthority;
    }
    aResult.mAuthority = LowerAscii(authority);
  }

  if (!path.empty()) {
    if (path.front() == '/' || path.back() == '/') {
      return AtURIError::InvalidPath;
    }
    auto segments = Split(path, '/');
    if (segments.size() > 2) {
      return AtURIError::InvalidPath;
    }
    for (auto segment : segments) {
      if (segment == "." || segment == "..") {
        return AtURIError::InvalidPath;
      }
    }

    std::string collection;
    if (!DecodeUnreserved(segments[0], collection) ||
        !IsValidNsid(collection)) {
      return AtURIError::InvalidCollection;
    }
    aResult.mCollection = NormalizeNsid(collection);
    aResult.mHasCollection = true;

    if (segments.size() == 2) {
      std::string recordKey;
      if (!DecodeUnreserved(segments[1], recordKey) ||
          !IsValidRecordKey(recordKey)) {
        return AtURIError::InvalidRecordKey;
      }
      aResult.mRecordKey = recordKey;
      aResult.mHasRecordKey = true;
    }
  }

  if (hash != std::string_view::npos) {
    std::string decoded;
    if (!DecodeUnreserved(fragment, decoded) ||
        !HasUppercaseHexTriplets(decoded)) {
      return AtURIError::InvalidPath;
    }
    for (char c : decoded) {
      if (c < 0x21 || c > 0x7e || c == '#') {
        return AtURIError::InvalidPath;
      }
    }
    aResult.mFragment = decoded;
    aResult.mHasFragment = true;
  }

  return AtURIError::None;
}

std::string NormalizeAtURI(std::string_view aSpec, AtURIError& aError) {
  AtURI uri;
  aError = ParseAtURI(aSpec, uri);
  if (aError != AtURIError::None) {
    return {};
  }
  std::string result = "at://" + uri.mAuthority;
  if (uri.mHasCollection) {
    result += "/" + uri.mCollection;
  }
  if (uri.mHasRecordKey) {
    result += "/" + uri.mRecordKey;
  }
  if (uri.mHasFragment) {
    result += "#" + uri.mFragment;
  }
  return result;
}

}  // namespace mozilla::net::atproto
