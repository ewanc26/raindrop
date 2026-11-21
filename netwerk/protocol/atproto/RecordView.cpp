/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "RecordView.h"

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
                             const RecordViewProvenance& aProvenance) {
  std::string html;
  html.reserve(aData.mValueJson.size() + 2048);

  html += "<!DOCTYPE html>\n<html><head><meta charset=\"utf-8\">\n";
  html += "<title>";
  html += EscapeHtml(aData.mLexicon.empty() ? aProvenance.mCollection
                                            : aData.mLexicon);
  html += "</title>\n<style>\n";
  html += "body{font:14px system-ui,sans-serif;margin:2em;max-width:60em}\n";
  html += "table{border-collapse:collapse;margin-bottom:1em}\n";
  html += "th,td{border:1px solid #ccc;padding:.3em .6em;text-align:left;"
         "vertical-align:top}\n";
  html += "th{background:#f4f4f4;white-space:nowrap}\n";
  html += "pre{white-space:pre-wrap;word-break:break-word}\n";
  html += ".hint{color:#555;font-size:.9em}\n";
  html += ".lexicon{margin:1em 0;padding:.5em 1em;background:#fafafa;border-left:3px solid #999}\n";
  html += ".langs{color:#555;font-size:.9em}\n";
  html += "</style></head><body>\n";

  html += "<h1>";
  html += EscapeHtml(aData.mLexicon.empty() ? aProvenance.mCollection
                                            : aData.mLexicon);
  html += "</h1>\n";

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

  if (aData.mCidStatus != CidStatus::Absent) {
    html += "<p class=\"hint\">CID verification checks the record against the "
            "CID returned with it — integrity only, not a repository "
            "signature or proof of authorship.</p>\n";
  }

  std::string summary = RenderLexiconSummary(aData.mLexicon, aData.mValueJson);
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

  html += "<h2>Raw JSON</h2>\n<pre>";
  html += EscapeHtml(aData.mRawJson);
  html += "</pre>\n";

  html += "</body></html>\n";
  return html;
}

}  // namespace mozilla::net::atproto
