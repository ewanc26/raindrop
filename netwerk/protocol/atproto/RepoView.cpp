/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "RepoView.h"

#include "AtDocumentPolicy.h"
#include "json/json.h"

namespace mozilla::net::atproto {

const char* RepoViewErrorName(RepoViewError aError) {
  switch (aError) {
    case RepoViewError::None:
      return "None";
    case RepoViewError::TooLong:
      return "TooLong";
    case RepoViewError::InvalidJson:
      return "InvalidJson";
    case RepoViewError::MissingDid:
      return "MissingDid";
  }
  return "Unknown";
}

RepoViewError ParseRepoResponse(std::string_view aJson, RepoViewData& aOut) {
  aOut = RepoViewData{};
  if (aJson.size() > kMaxRepoResponseLength) {
    return RepoViewError::TooLong;
  }

  Json::CharReaderBuilder builder;
  std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
  Json::Value root;
  std::string errors;
  if (!reader->parse(aJson.data(), aJson.data() + aJson.size(), &root,
                     &errors) ||
      !root.isObject()) {
    return RepoViewError::InvalidJson;
  }

  const Json::Value& did = root["did"];
  if (!did.isString()) {
    return RepoViewError::MissingDid;
  }
  aOut.mDid = did.asString();

  const Json::Value& handle = root["handle"];
  if (handle.isString()) {
    aOut.mHandle = handle.asString();
  }

  const Json::Value& collections = root["collections"];
  if (collections.isArray()) {
    for (const Json::Value& collection : collections) {
      if (collection.isString()) {
        aOut.mCollections.push_back(collection.asString());
      }
    }
  }
  return RepoViewError::None;
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

std::string RenderRepoHtml(const RepoViewData& aData,
                           const RepoViewProvenance& aProvenance,
                           const ViewStyleOptions& aStyle) {
  std::string html;
  html.reserve(2048);

  html += "<!DOCTYPE html>\n<html><head><meta charset=\"utf-8\">\n";
  AppendDocumentSecurityMeta(html);
  html += "<title>Repository — ";
  html += EscapeHtml(aProvenance.mHandle.empty() ? aProvenance.mDid
                                                 : aProvenance.mHandle);
  html += "</title>\n";
  AppendDocumentStyle(html, aStyle);
  html += "</head><body>\n";

  DocumentHeader header;
  header.mRequested = aProvenance.mHandle;
  header.mDid = aProvenance.mDid;
  header.mPds = aProvenance.mPds;
  AppendDocumentHeader(html, header);

  html += "<h1>Repository summary</h1>\n<table>\n";
  AppendMeta(html, "Handle",
             aData.mHandle.empty() ? aProvenance.mHandle : aData.mHandle);
  AppendMeta(html, "DID", aData.mDid.empty() ? aProvenance.mDid : aData.mDid);
  AppendMeta(html, "Repository server", aProvenance.mPds);
  html += "</table>\n";

  html += "<h2>Collections</h2>\n";
  if (aData.mCollections.empty()) {
    html += "<p>(none advertised)</p>\n";
  } else {
    html += "<ul>\n";
    for (const std::string& collection : aData.mCollections) {
      html += "<li><a href=\"at://";
      html +=
          EscapeHtml(aProvenance.mDid.empty() ? aData.mDid : aProvenance.mDid);
      html += "/";
      html += EscapeHtml(collection);
      html += "\"><code>";
      html += EscapeHtml(collection);
      html += "</code></a></li>\n";
    }
    html += "</ul>\n";
  }

  html +=
      "<p class=\"hint\">Raindrop resolved this repository directly from its "
      "DID document and repository server. No gateway was contacted. "
      "Collections are those advertised by the repository.</p>\n";
  html += "</body></html>\n";
  return html;
}

}  // namespace mozilla::net::atproto
