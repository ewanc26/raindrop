/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "CollectionView.h"

#include <memory>
#include <utility>

#include "AtURI.h"
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

void AppendMeta(std::string& aHtml, const char* aLabel, std::string_view aValue) {
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

std::string RenderCollectionHtml(
    const CollectionViewData& aData,
    const CollectionViewProvenance& aProvenance) {
  std::string html;
  html.reserve(4096);

  html += "<!DOCTYPE html>\n<html><head><meta charset=\"utf-8\">\n";
  html += "<title>Collection — ";
  html += EscapeHtml(aProvenance.mCollection);
  html += "</title>\n<style>\n";
  html += "body{font:14px system-ui,sans-serif;margin:2em;max-width:60em}\n";
  html += "table{border-collapse:collapse;margin-bottom:1em}\n";
  html +=
      "th,td{border:1px solid #ccc;padding:.3em .6em;text-align:left;"
      "vertical-align:top}\n";
  html += "th{background:#f4f4f4;white-space:nowrap}\n";
  html += "code{word-break:break-all}\n";
  html += "li{margin-bottom:.4em}\n";
  html += ".hint{color:#555;font-size:.9em}\n";
  html += "</style></head><body>\n";

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
      html += "</li>\n";
    }
    html += "</ol>\n";
  }

  if (!aData.mCursor.empty()) {
    html +=
        "<p class=\"hint\">More records are available; page navigation is not "
        "implemented yet.</p>\n";
  }
  html +=
      "<p class=\"hint\">Raindrop listed this collection directly from its "
      "repository server. No gateway was contacted.</p>\n";
  html += "</body></html>\n";
  return html;
}

}  // namespace mozilla::net::atproto
