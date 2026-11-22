/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "CollectionView.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "AtDocumentPolicy.h"
#include "AtURI.h"
#include "PdsClient.h"
#include "json/json.h"

namespace mozilla::net::atproto {

const char* CollectionViewErrorName(CollectionViewError aError) {
  switch (aError) {
    case CollectionViewError::None:
      return "None";
    case CollectionViewError::TooLong:
      return "TooLong";
    case CollectionViewError::InvalidJson:
      return "InvalidJson";
    case CollectionViewError::MissingRecords:
      return "MissingRecords";
    case CollectionViewError::InvalidRecord:
      return "InvalidRecord";
  }
  return "Unknown";
}

namespace {

constexpr size_t kMaxSummaryBytes = 160;
constexpr size_t kMaxWhenBytes = 40;

bool IsControlByte(unsigned char aByte) {
  return aByte < 0x20 || aByte == 0x7f;
}

bool IsUtf8Continuation(unsigned char aByte) { return (aByte & 0xc0) == 0x80; }

std::string SanitizeSummary(std::string_view aText, size_t aMaxBytes) {
  if (aText.empty()) {
    return {};
  }
  std::string clean;
  clean.reserve(std::min(aText.size(), aMaxBytes + 3));
  for (unsigned char c : aText) {
    clean += IsControlByte(c) ? ' ' : static_cast<char>(c);
  }
  auto begin = clean.find_first_not_of(' ');
  if (begin == std::string::npos) {
    return {};
  }
  auto end = clean.find_last_not_of(' ');
  std::string_view value(clean.data() + begin, end - begin + 1);
  if (value.size() <= aMaxBytes) {
    return std::string(value);
  }
  size_t cut = aMaxBytes;
  while (cut > 0 && cut < value.size() &&
         IsUtf8Continuation(static_cast<unsigned char>(value[cut]))) {
    --cut;
  }
  return std::string(value.substr(0, cut)) + "\xe2\x80\xa6";
}

void SetRecordSummary(const Json::Value& aValue,
                      CollectionViewRecord& aRecord) {
  if (!aValue.isObject()) {
    return;
  }
  const Json::Value& type = aValue["$type"];
  if (type.isString() && IsValidNsid(type.asString())) {
    aRecord.mSummaryType = type.asString();
  }

  constexpr const char* kSummaryKeys[] = {"text", "displayName", "description",
                                          "name", "title"};
  for (const char* key : kSummaryKeys) {
    const Json::Value& field = aValue[key];
    if (!field.isString()) {
      continue;
    }
    aRecord.mSummaryText = SanitizeSummary(field.asString(), kMaxSummaryBytes);
    if (!aRecord.mSummaryText.empty()) {
      break;
    }
  }

  const Json::Value& created = aValue["createdAt"];
  if (created.isString()) {
    aRecord.mSummaryWhen = SanitizeSummary(created.asString(), kMaxWhenBytes);
  }
}

}  // namespace

CollectionViewError ParseCollectionResponse(std::string_view aJson,
                                            CollectionViewData& aOut) {
  aOut = CollectionViewData{};
  if (aJson.size() > kMaxCollectionResponseLength) {
    return CollectionViewError::TooLong;
  }

  Json::CharReaderBuilder builder;
  std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
  Json::Value root;
  std::string errors;
  if (!reader->parse(aJson.data(), aJson.data() + aJson.size(), &root,
                     &errors) ||
      !root.isObject()) {
    return CollectionViewError::InvalidJson;
  }

  const Json::Value& records = root["records"];
  if (!records.isArray()) {
    return CollectionViewError::MissingRecords;
  }
  for (const Json::Value& entry : records) {
    if (!entry.isObject()) {
      return CollectionViewError::InvalidRecord;
    }
    const Json::Value& uri = entry["uri"];
    if (!uri.isString()) {
      return CollectionViewError::InvalidRecord;
    }
    AtURI parsed;
    if (ParseAtURI(uri.asString(), parsed) != AtURIError::None ||
        !parsed.mHasCollection || !parsed.mHasRecordKey) {
      return CollectionViewError::InvalidRecord;
    }
    CollectionViewRecord record;
    record.mRecordKey = parsed.mRecordKey;
    const Json::Value& cid = entry["cid"];
    if (cid.isString()) {
      record.mCid = cid.asString();
    }
    SetRecordSummary(entry["value"], record);
    aOut.mRecords.push_back(std::move(record));
  }

  const Json::Value& cursor = root["cursor"];
  if (cursor.isString()) {
    aOut.mCursor = cursor.asString();
  }
  return CollectionViewError::None;
}

namespace {

std::string EscapeHtml(std::string_view aText) {
  std::string out;
  out.reserve(aText.size());
  for (char c : aText) {
    switch (c) {
      case '&':
        out += "&amp;";
        break;
      case '<':
        out += "&lt;";
        break;
      case '>':
        out += "&gt;";
        break;
      case '"':
        out += "&quot;";
        break;
      case '\'':
        out += "&#39;";
        break;
      default:
        out += c;
    }
  }
  return out;
}

void AppendRecordSummary(std::string& aHtml,
                         const CollectionViewRecord& aRecord) {
  if (aRecord.mSummaryType.empty() && aRecord.mSummaryText.empty() &&
      aRecord.mSummaryWhen.empty()) {
    return;
  }
  aHtml += "<div class=\"hint\">";
  if (!aRecord.mSummaryType.empty()) {
    aHtml += EscapeHtml(aRecord.mSummaryType);
    if (!aRecord.mSummaryText.empty() || !aRecord.mSummaryWhen.empty()) {
      aHtml += " — ";
    }
  }
  if (!aRecord.mSummaryText.empty()) {
    aHtml += EscapeHtml(aRecord.mSummaryText);
  }
  if (!aRecord.mSummaryWhen.empty()) {
    if (!aRecord.mSummaryText.empty()) {
      aHtml += " ";
    }
    aHtml += "(";
    aHtml += EscapeHtml(aRecord.mSummaryWhen);
    aHtml += ")";
  }
  aHtml += "</div>\n";
}

void AppendMeta(std::string& aHtml, const char* aLabel,
                std::string_view aValue) {
  if (aValue.empty()) {
    return;
  }
  aHtml += "<tr><th>";
  aHtml += aLabel;
  aHtml += "</th><td><code>";
  aHtml += EscapeHtml(aValue);
  aHtml += "</code></td></tr>\n";
}

}  // namespace

std::string RenderCollectionHtml(const CollectionViewData& aData,
                                 const CollectionViewProvenance& aProvenance,
                                 const ViewStyleOptions& aStyle) {
  std::string html;
  html.reserve(4096);

  html += "<!DOCTYPE html>\n<html><head><meta charset=\"utf-8\">\n";
  AppendDocumentSecurityMeta(html);
  html += "<title>Collection — ";
  html += EscapeHtml(aProvenance.mCollection);
  html += "</title>\n";
  AppendDocumentStyle(html, aStyle);
  html += "</head><body>\n";

  html += "<h1>Collection</h1>\n<table>\n";
  AppendMeta(html, "Collection", aProvenance.mCollection);
  AppendMeta(html, "Handle", aProvenance.mHandle);
  AppendMeta(html, "DID", aProvenance.mDid);
  AppendMeta(html, "Repository server", aProvenance.mPds);
  html += "</table>\n";

  html += "<h2>Records</h2>\n";
  if (aData.mRecords.empty()) {
    html += "<p>(no records)</p>\n";
  } else {
    html += "<ol>\n";
    for (const CollectionViewRecord& record : aData.mRecords) {
      html += "<li><a href=\"at://";
      html += EscapeHtml(aProvenance.mDid);
      html += "/";
      html += EscapeHtml(aProvenance.mCollection);
      html += "/";
      html += EscapeHtml(record.mRecordKey);
      html += "\">";
      html += EscapeHtml(record.mRecordKey);
      html += "</a>";
      if (!record.mCid.empty()) {
        html += " <code>";
        html += EscapeHtml(record.mCid);
        html += "</code>";
      }
      AppendRecordSummary(html, record);
      html += "</li>\n";
    }
    html += "</ol>\n";
  }

  std::string base = "at://";
  base += aProvenance.mDid;
  base += "/";
  base += aProvenance.mCollection;

  if (!aData.mCursor.empty()) {
    html += "<p><a href=\"";
    html += EscapeHtml(base);
    html += "/cursor/";
    html += EscapeHtml(EncodeQueryValue(aData.mCursor));
    html += "\">Next page</a></p>\n";
  }
  if (!aProvenance.mCursor.empty()) {
    html += "<p><a href=\"";
    html += EscapeHtml(base);
    html += "\">First page</a></p>\n";
  }

  html +=
      "<p class=\"hint\">Raindrop listed this collection directly from its "
      "repository server. No gateway was contacted.</p>\n";
  html += "</body></html>\n";
  return html;
}

}  // namespace mozilla::net::atproto
