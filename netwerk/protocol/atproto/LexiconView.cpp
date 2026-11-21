/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "LexiconView.h"

#include <memory>

#include "json/json.h"

namespace mozilla::net::atproto {

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

void AppendString(std::string& aHtml, const Json::Value& aValue,
                  const char* aKey, const char* aLabel) {
  const Json::Value& field = aValue[aKey];
  if (!field.isString() || field.asString().empty()) {
    return;
  }
  aHtml += "<tr><th>";
  aHtml += aLabel;
  aHtml += "</th><td>";
  aHtml += EscapeHtml(field.asString());
  aHtml += "</td></tr>\n";
}

std::string RenderPost(const Json::Value& aValue) {
  std::string html;
  html += "<section class=\"lexicon\">\n<table>\n";
  AppendString(html, aValue, "text", "Text");
  AppendString(html, aValue, "createdAt", "Posted");
  html += "</table>\n";
  const Json::Value& langs = aValue["langs"];
  if (langs.isArray() && !langs.empty()) {
    html += "<p class=\"langs\">Languages: ";
    bool first = true;
    for (const Json::Value& lang : langs) {
      if (!lang.isString()) {
        continue;
      }
      if (!first) {
        html += ", ";
      }
      html += EscapeHtml(lang.asString());
      first = false;
    }
    html += "</p>\n";
  }
  html += "</section>\n";
  return html;
}

std::string RenderProfile(const Json::Value& aValue) {
  std::string html;
  html += "<section class=\"lexicon\">\n<table>\n";
  AppendString(html, aValue, "displayName", "Name");
  AppendString(html, aValue, "description", "Description");
  html += "</table>\n</section>\n";
  return html;
}

}  // namespace

std::string RenderLexiconSummary(std::string_view aLexicon,
                                 std::string_view aValueJson) {
  if (aLexicon != "app.bsky.feed.post" && aLexicon != "app.bsky.actor.profile") {
    return {};
  }

  Json::CharReaderBuilder builder;
  std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
  Json::Value value;
  std::string errors;
  if (!reader->parse(aValueJson.data(), aValueJson.data() + aValueJson.size(),
                     &value, &errors) ||
      !value.isObject()) {
    return {};
  }

  return aLexicon == "app.bsky.feed.post" ? RenderPost(value)
                                         : RenderProfile(value);
}

}  // namespace mozilla::net::atproto
