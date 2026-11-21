/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DidResolver.h"

#include "AtURI.h"
#include "json/json.h"

#include <vector>

namespace mozilla::net::atproto {

namespace {

constexpr size_t kMaxDidLength = 2048;
constexpr size_t kMaxPlcIdLength = 64;
constexpr size_t kMaxEndpointLength = 2048;

bool IsDigit(char aChar) { return aChar >= '0' && aChar <= '9'; }

bool IsAlpha(char aChar) {
  return (aChar >= 'a' && aChar <= 'z') || (aChar >= 'A' && aChar <= 'Z');
}

bool IsAlnum(char aChar) { return IsAlpha(aChar) || IsDigit(aChar); }

bool IsHexDigit(char aChar) {
  return IsDigit(aChar) || (aChar >= 'a' && aChar <= 'f') ||
         (aChar >= 'A' && aChar <= 'F');
}

int HexValue(char aChar) {
  if (IsDigit(aChar)) {
    return aChar - '0';
  }
  return (aChar >= 'a' ? aChar - 'a' : aChar - 'A') + 10;
}

char ToLowerAscii(char aChar) {
  return aChar >= 'A' && aChar <= 'Z' ? aChar - 'A' + 'a' : aChar;
}

char ToUpperAscii(char aChar) {
  return aChar >= 'a' && aChar <= 'z' ? aChar - 'a' + 'A' : aChar;
}

// DID percent escapes use case-insensitive hex digits; canonicalize them to
// uppercase so a DID read from a handle's TXT/well-known response compares equal
// to the canonical id in the DID document (RFC 3986 section 6.2.2.1).
std::string CanonicalizeDid(std::string_view aDid) {
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

bool EndsWithIgnoreCase(std::string_view aValue, std::string_view aSuffix) {
  if (aValue.size() < aSuffix.size()) {
    return false;
  }
  size_t start = aValue.size() - aSuffix.size();
  for (size_t i = 0; i < aSuffix.size(); ++i) {
    if (ToLowerAscii(aValue[start + i]) != ToLowerAscii(aSuffix[i])) {
      return false;
    }
  }
  return true;
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

// Decode percent escapes in one DID segment, rejecting malformed escapes.
// Callers must validate the decoded bytes with IsValidWebHost or
// IsValidWebPathSegment before using them.
bool DecodeSegment(std::string_view aInput, std::string& aOutput) {
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
    aOutput.push_back(
        static_cast<char>(HexValue(aInput[i + 1]) * 16 + HexValue(aInput[i + 2])));
    i += 2;
  }
  return true;
}

bool IsValidLabel(std::string_view aLabel) {
  if (aLabel.empty() || aLabel.size() > 63) {
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

// Validate a decoded did:web host, including an optional numeric port.
bool IsValidWebHost(std::string_view aHost) {
  auto colon = aHost.find(':');
  std::string_view domain =
      colon == std::string_view::npos ? aHost : aHost.substr(0, colon);
  if (domain.empty()) {
    return false;
  }
  auto labels = Split(domain, '.');
  if (labels.size() < 2) {
    return false;
  }
  for (auto label : labels) {
    if (!IsValidLabel(label)) {
      return false;
    }
  }
  if (colon != std::string_view::npos) {
    auto port = aHost.substr(colon + 1);
    if (port.empty() || port.size() > 5) {
      return false;
    }
    for (char c : port) {
      if (!IsDigit(c)) {
        return false;
      }
    }
    int portNumber = 0;
    for (char c : port) {
      portNumber = portNumber * 10 + (c - '0');
    }
    if (portNumber < 1 || portNumber > 65535) {
      return false;
    }
  }
  return true;
}

bool IsValidWebPathSegment(std::string_view aSegment) {
  if (aSegment.empty() || aSegment == "." || aSegment == "..") {
    return false;
  }
  for (char c : aSegment) {
    if (!IsAlnum(c) && c != '.' && c != '_' && c != '-' && c != '~') {
      return false;
    }
  }
  return true;
}

bool IsValidPlcId(std::string_view aId) {
  if (aId.empty() || aId.size() > kMaxPlcIdLength) {
    return false;
  }
  for (char c : aId) {
    if (ToLowerAscii(c) < 'a' || ToLowerAscii(c) > 'z') {
      if (!IsDigit(c)) {
        return false;
      }
    }
  }
  return true;
}

// Return the authority component of an http(s) URL, or an empty view if the
// URL does not start with a supported scheme.
std::string_view EndpointAuthority(std::string_view aEndpoint) {
  constexpr std::string_view https = "https://";
  constexpr std::string_view http = "http://";
  std::string_view rest;
  if (aEndpoint.substr(0, https.size()) == https) {
    rest = aEndpoint.substr(https.size());
  } else if (aEndpoint.substr(0, http.size()) == http) {
    rest = aEndpoint.substr(http.size());
  } else {
    return {};
  }
  auto slash = rest.find('/');
  return slash == std::string_view::npos ? rest : rest.substr(0, slash);
}

// Require an absolute https URL with a host and no credentials. The trust
// model does not permit gateway fallbacks, so plain http is rejected here.
bool IsValidEndpoint(std::string_view aEndpoint) {
  if (aEndpoint.empty() || aEndpoint.size() > kMaxEndpointLength) {
    return false;
  }
  constexpr std::string_view scheme = "https://";
  if (aEndpoint.substr(0, scheme.size()) != scheme) {
    return false;
  }
  std::string_view authority = EndpointAuthority(aEndpoint);
  if (authority.empty() || authority.find('@') != std::string_view::npos) {
    return false;
  }
  return true;
}

// Onion-only PDS endpoints are a distinct failure state (ADR 0004): the load
// must report that native Tor support is unavailable rather than treating the
// endpoint as a generic malformed URL or proxying it through a gateway.
bool IsOnionEndpoint(std::string_view aEndpoint) {
  std::string_view authority = EndpointAuthority(aEndpoint);
  auto colon = authority.find(':');
  std::string_view host =
      colon == std::string_view::npos ? authority : authority.substr(0, colon);
  constexpr std::string_view suffix = ".onion";
  return host.size() > suffix.size() && EndsWithIgnoreCase(host, suffix);
}

}  // namespace

const char* DidMethodName(DidMethod aMethod) {
  switch (aMethod) {
    case DidMethod::Unknown:
      return "Unknown";
    case DidMethod::Plc:
      return "Plc";
    case DidMethod::Web:
      return "Web";
  }
  return "Unknown";
}

const char* DidErrorName(DidError aError) {
  switch (aError) {
    case DidError::None:
      return "None";
    case DidError::TooLong:
      return "TooLong";
    case DidError::InvalidSyntax:
      return "InvalidSyntax";
    case DidError::UnsupportedMethod:
      return "UnsupportedMethod";
    case DidError::InvalidIdentifier:
      return "InvalidIdentifier";
  }
  return "Unknown";
}

const char* DidUrlErrorName(DidUrlError aError) {
  switch (aError) {
    case DidUrlError::None:
      return "None";
    case DidUrlError::InvalidDid:
      return "InvalidDid";
    case DidUrlError::InvalidWebHost:
      return "InvalidWebHost";
    case DidUrlError::InvalidWebPath:
      return "InvalidWebPath";
  }
  return "Unknown";
}

const char* PdsErrorName(PdsError aError) {
  switch (aError) {
    case PdsError::None:
      return "None";
    case PdsError::TooLarge:
      return "TooLarge";
    case PdsError::InvalidJson:
      return "InvalidJson";
    case PdsError::IdMismatch:
      return "IdMismatch";
    case PdsError::MissingId:
      return "MissingId";
    case PdsError::MissingService:
      return "MissingService";
    case PdsError::InvalidService:
      return "InvalidService";
    case PdsError::InvalidEndpoint:
      return "InvalidEndpoint";
    case PdsError::OnionEndpoint:
      return "OnionEndpoint";
  }
  return "Unknown";
}

const char* HandleBindingErrorName(HandleBindingError aError) {
  switch (aError) {
    case HandleBindingError::None:
      return "None";
    case HandleBindingError::InvalidJson:
      return "InvalidJson";
    case HandleBindingError::MissingAlsoKnownAs:
      return "MissingAlsoKnownAs";
    case HandleBindingError::HandleNotClaimed:
      return "HandleNotClaimed";
  }
  return "Unknown";
}

HandleBindingError VerifyHandleBinding(std::string_view aJson,
                                       std::string_view aHandle) {
  if (aJson.size() > kMaxDidDocumentLength) {
    return HandleBindingError::InvalidJson;
  }

  Json::CharReaderBuilder builder;
  std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
  Json::Value root;
  std::string errors;
  if (!reader->parse(aJson.data(), aJson.data() + aJson.size(), &root,
                     &errors) ||
      !root.isObject()) {
    return HandleBindingError::InvalidJson;
  }

  const Json::Value& alsoKnownAs = root["alsoKnownAs"];
  if (!alsoKnownAs.isArray()) {
    return HandleBindingError::MissingAlsoKnownAs;
  }

  const std::string expected = "at://" + std::string(aHandle);
  for (const auto& entry : alsoKnownAs) {
    if (!entry.isString()) {
      continue;
    }
    const std::string candidate = entry.asString();
    if (candidate.size() == expected.size()) {
      bool equal = true;
      for (size_t i = 0; i < expected.size(); ++i) {
        if (ToLowerAscii(candidate[i]) != ToLowerAscii(expected[i])) {
          equal = false;
          break;
        }
      }
      if (equal) {
        return HandleBindingError::None;
      }
    }
  }
  return HandleBindingError::HandleNotClaimed;
}

DidError ParseDid(std::string_view aDid, Did& aResult) {
  aResult = Did{};
  if (aDid.size() > kMaxDidLength || !IsValidDid(aDid)) {
    return aDid.size() > kMaxDidLength ? DidError::TooLong
                                       : DidError::InvalidSyntax;
  }

  auto remainder = aDid.substr(4);
  auto separator = remainder.find(':');
  std::string_view method = remainder.substr(0, separator);
  std::string_view id = remainder.substr(separator + 1);

  if (method == "plc") {
    if (!IsValidPlcId(id)) {
      return DidError::InvalidIdentifier;
    }
    aResult.mMethod = DidMethod::Plc;
  } else if (method == "web") {
    auto segments = Split(id, ':');
    std::string host;
    if (!DecodeSegment(segments.front(), host) || !IsValidWebHost(host)) {
      return DidError::InvalidIdentifier;
    }
    for (size_t i = 1; i < segments.size(); ++i) {
      std::string segment;
      if (!DecodeSegment(segments[i], segment) ||
          !IsValidWebPathSegment(segment)) {
        return DidError::InvalidIdentifier;
      }
    }
    aResult.mMethod = DidMethod::Web;
  } else {
    return DidError::UnsupportedMethod;
  }

  aResult.mFull = CanonicalizeDid(aDid);
  aResult.mId = CanonicalizeDid(id);
  return DidError::None;
}

DidUrlError BuildDidDocumentUrl(std::string_view aDid, std::string& aUrl) {
  aUrl.clear();
  Did did;
  DidError error = ParseDid(aDid, did);
  if (error != DidError::None) {
    return DidUrlError::InvalidDid;
  }

  if (did.mMethod == DidMethod::Plc) {
    aUrl = "https://plc.directory/" + did.mFull;
    return DidUrlError::None;
  }

  auto segments = Split(did.mId, ':');
  std::string host;
  if (!DecodeSegment(segments.front(), host) || !IsValidWebHost(host)) {
    return DidUrlError::InvalidWebHost;
  }
  aUrl = "https://" + host;
  if (segments.size() == 1) {
    aUrl += "/.well-known/did.json";
    return DidUrlError::None;
  }
  for (size_t i = 1; i < segments.size(); ++i) {
    std::string segment;
    if (!DecodeSegment(segments[i], segment) ||
        !IsValidWebPathSegment(segment)) {
      aUrl.clear();
      return DidUrlError::InvalidWebPath;
    }
    aUrl += "/" + segment;
  }
  aUrl += "/did.json";
  return DidUrlError::None;
}

PdsError ExtractPdsEndpoint(std::string_view aJson,
                            std::string_view aExpectedDid,
                            PdsService& aResult) {
  aResult = PdsService{};
  if (aJson.size() > kMaxDidDocumentLength) {
    return PdsError::TooLarge;
  }

  Json::CharReaderBuilder builder;
  std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
  Json::Value root;
  std::string errors;
  if (!reader->parse(aJson.data(), aJson.data() + aJson.size(), &root,
                     &errors) ||
      !root.isObject()) {
    return PdsError::InvalidJson;
  }

  const Json::Value& id = root["id"];
  if (!id.isString()) {
    return PdsError::MissingId;
  }
  std::string canonicalExpected = CanonicalizeDid(aExpectedDid);
  if (CanonicalizeDid(id.asString()) != canonicalExpected) {
    return PdsError::IdMismatch;
  }

  const Json::Value& services = root["service"];
  if (!services.isArray()) {
    return PdsError::MissingService;
  }

  const std::string fragment = "#atproto_pds";
  const std::string fullId = canonicalExpected + fragment;
  for (const auto& service : services) {
    if (!service.isObject()) {
      continue;
    }
    const Json::Value& serviceId = service["id"];
    if (!serviceId.isString()) {
      continue;
    }
    const std::string candidate = serviceId.asString();
    if (candidate != fragment && candidate != fullId) {
      continue;
    }
    const Json::Value& endpoint = service["serviceEndpoint"];
    if (!endpoint.isString()) {
      return PdsError::InvalidEndpoint;
    }
    if (IsOnionEndpoint(endpoint.asString())) {
      return PdsError::OnionEndpoint;
    }
    if (!IsValidEndpoint(endpoint.asString())) {
      return PdsError::InvalidEndpoint;
    }
    aResult.mEndpoint = endpoint.asString();
    return PdsError::None;
  }
  return PdsError::MissingService;
}

}  // namespace mozilla::net::atproto
