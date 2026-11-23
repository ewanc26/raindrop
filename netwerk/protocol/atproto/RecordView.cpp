/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "RecordView.h"

#include "AtDocumentPolicy.h"
#include "LexiconView.h"
#include "json/json.h"

namespace mozilla::net::atproto {

const char* RecordViewErrorName(RecordViewError aError) {
  switch (aError) {
    case RecordViewError::None:
      return "None";
    case RecordViewError::TooLong:
      return "TooLong";
    case RecordViewError::InvalidJson:
      return "InvalidJson";
    case RecordViewError::InvalidEnvelope:
      return "InvalidEnvelope";
    case RecordViewError::MissingValue:
      return "MissingValue";
  }
  return "Unknown";
}

RecordViewError ParseRecordResponse(std::string_view aJson,
                                    RecordViewData& aOut) {
  aOut = RecordViewData{};
  if (aJson.size() > kMaxRecordResponseLength) {
    return RecordViewError::TooLong;
  }

  Json::CharReaderBuilder builder;
  std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
  Json::Value root;
  std::string errors;
  if (!reader->parse(aJson.data(), aJson.data() + aJson.size(), &root,
                     &errors) ||
      !root.isObject()) {
    return RecordViewError::InvalidJson;
  }

  const Json::Value& value = root["value"];
  if (!value.isObject()) {
    return RecordViewError::MissingValue;
  }

  aOut.mRawJson.assign(aJson.data(), aJson.size());

  const Json::Value& cid = root["cid"];
  aOut.mCidStatus = CidStatus::Absent;
  if (cid.isString()) {
    aOut.mCid = cid.asString();
    aOut.mCidStatus = VerifyRecordCid(aOut.mCid, value);
  }

  const Json::Value& type = value["$type"];
  if (type.isString()) {
    aOut.mLexicon = type.asString();
  }

  Json::StreamWriterBuilder writer;
  writer["indentation"] = "  ";
  aOut.mValueJson = Json::writeString(writer, value);

  for (const std::string& name : value.getMemberNames()) {
    RecordField field;
    field.mName = name;
    field.mValue = Json::writeString(writer, value[name]);
    aOut.mFields.push_back(std::move(field));
  }

  return RecordViewError::None;
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
  aHtml += "<tr><th>";
  aHtml += aLabel;
  aHtml += "</th><td>";
  aHtml += EscapeHtml(aValue);
  aHtml += "</td></tr>\n";
}

}  // namespace

std::string RenderRecordHtml(const RecordViewData& aData,
                             const RecordViewProvenance& aProvenance,
                             const RecordViewOptions& aOptions,
                             const ViewStyleOptions& aStyle) {
  std::string html;
  html.reserve(aData.mValueJson.size() + 2048);

  html += "<!DOCTYPE html>\n<html><head><meta charset=\"utf-8\">\n";
  AppendDocumentSecurityMeta(html);
  html += "<title>";
  html += EscapeHtml(aData.mLexicon.empty() ? aProvenance.mCollection
                                            : aData.mLexicon);
  html += "</title>\n";
  AppendDocumentStyle(html, aStyle);
  html += "</head><body>\n";

  DocumentHeader header;
  if (aOptions.mShowProvenance) {
    header.mRequested = aProvenance.mHandle;
    header.mDid = aProvenance.mDid;
    header.mPds = aProvenance.mPds;
    header.mCollection = aProvenance.mCollection;
    header.mRecordKey = aProvenance.mRecordKey;
    header.mCid = aData.mCid;
    if (aData.mCidStatus != CidStatus::Absent) {
      header.mCidStatus = CidStatusName(aData.mCidStatus);
    }
  }
  AppendDocumentHeader(html, header);

  html += "<h1>";
  html += EscapeHtml(aData.mLexicon.empty() ? aProvenance.mCollection
                                            : aData.mLexicon);
  html += "</h1>\n";

  if (aOptions.mShowProvenance) {
    html += "<h2>Provenance</h2>\n<table>\n";
    AppendMeta(html, "Collection", aProvenance.mCollection);
    AppendMeta(html, "Record key", aProvenance.mRecordKey);
    AppendMeta(html, "Lexicon", aData.mLexicon);
    AppendMeta(html, "DID", aProvenance.mDid);
    if (!aProvenance.mHandle.empty()) {
      AppendMeta(html, "Handle", aProvenance.mHandle);
    }
    AppendMeta(html, "PDS", aProvenance.mPds);
    AppendMeta(html, "CID", aData.mCid);
    AppendMeta(html, "CID verification", CidStatusName(aData.mCidStatus));
    html += "</table>\n";

    if (!aProvenance.mHandle.empty()) {
      html += "<p class=\"hint\">Handle — may change; resolved to ";
      html += EscapeHtml(aProvenance.mDid);
      html += ".</p>\n";
    }

    html +=
        "<p class=\"hint\">Provenance limit: Raindrop does not verify "
        "repository commit signatures or signed commit proofs. A matching "
        "CID does not establish who authored this record.</p>\n";

    if (aData.mCidStatus != CidStatus::Absent) {
      html +=
          "<p class=\"hint\">CID verification checks the record against "
          "the CID returned with it. This confirms integrity and consistency "
          "with the PDS response, not authorship or repository-commit "
          "authenticity.</p>\n";
    }
  }

  std::string summary = RenderLexiconSummary(
      aData.mLexicon, aData.mValueJson,
      LexiconViewProvenance{aProvenance.mDid, aProvenance.mPds,
                            aOptions.mShowImages});
  if (!summary.empty()) {
    html += "<h2>Record</h2>\n";
    html += summary;
  }

  html += "<h2>Fields</h2>\n";
  if (aData.mFields.empty()) {
    html += "<p>(none)</p>\n";
  } else {
    html += "<table>\n";
    for (const RecordField& field : aData.mFields) {
      html += "<tr><th>";
      html += EscapeHtml(field.mName);
      html += "</th><td><pre>";
      html += EscapeHtml(field.mValue);
      html += "</pre></td></tr>\n";
    }
    html += "</table>\n";
  }

  if (aOptions.mShowRawJson) {
    html += "<h2>Raw JSON</h2>\n<pre>";
    html += EscapeHtml(aData.mRawJson);
    html += "</pre>\n";
  }

  html += "</body></html>\n";
  return html;
}

}  // namespace mozilla::net::atproto
