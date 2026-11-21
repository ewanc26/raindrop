/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "LexiconView.h"

#include <memory>

#include "AtURI.h"
#include "PdsClient.h"
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

void AppendBlob(std::string& aHtml, std::string_view aLabel,
                const Json::Value& aBlob, const Json::Value& aAlt,
                const LexiconViewProvenance& aProvenance) {
  if (!aBlob.isObject()) {
    return;
  }
  aHtml += "<li><strong>";
  aHtml += EscapeHtml(aLabel);
  aHtml += "</strong>: ";
  const Json::Value& mimeType = aBlob["mimeType"];
  if (mimeType.isString() && !mimeType.asString().empty()) {
    aHtml += EscapeHtml(mimeType.asString());
  }
  const Json::Value& size = aBlob["size"];
  if (size.isUInt()) {
    aHtml += " (" + std::to_string(size.asUInt()) + " bytes)";
  }
  const Json::Value& ref = aBlob["ref"];
  const Json::Value& link =
      ref.isObject() && ref.isMember("$link") ? ref["$link"] : aBlob["cid"];
  std::string url;
  bool hasUrl = link.isString() && !link.asString().empty() &&
                BuildGetBlobUrl(aProvenance.mPds, aProvenance.mDid,
                                link.asString(), url) == PdsUrlError::None;
  if (hasUrl && aProvenance.mShowImages && mimeType.isString() &&
      mimeType.asString().rfind("image/", 0) == 0) {
    aHtml += "<img class=\"blob\" src=\"";
    aHtml += EscapeHtml(url);
    aHtml += "\" alt=\"";
    if (aAlt.isString()) {
      aHtml += EscapeHtml(aAlt.asString());
    }
    aHtml += "\" loading=\"lazy\">";
  }
  if (link.isString() && !link.asString().empty()) {
    aHtml += " — CID <code>";
    aHtml += EscapeHtml(link.asString());
    aHtml += "</code>";
    if (hasUrl) {
      aHtml += " <a href=\"";
      aHtml += EscapeHtml(url);
      aHtml += "\" rel=\"noreferrer\">Open</a>";
    }
  }
  if (aAlt.isString() && !aAlt.asString().empty()) {
    aHtml += " — ";
    aHtml += EscapeHtml(aAlt.asString());
  }
  aHtml += "</li>\n";
}

void AppendImages(std::string& aHtml, const Json::Value& aImages,
                  const LexiconViewProvenance& aProvenance) {
  if (!aImages.isArray() || aImages.empty()) {
    return;
  }
  std::string items;
  for (const Json::Value& image : aImages) {
    if (image.isObject()) {
      AppendBlob(items, "Image", image["image"], image["alt"], aProvenance);
    }
  }
  if (items.empty()) {
    return;
  }
  aHtml += "<div class=\"blobs\">Attachments: <ul>\n";
  aHtml += items;
  aHtml += "</ul></div>\n";
}

void AppendReference(std::string& aHtml, std::string_view aLabel,
                     const Json::Value& aRecord) {
  if (!aRecord.isObject()) {
    return;
  }
  const Json::Value& uri = aRecord["uri"];
  if (!uri.isString()) {
    return;
  }
  AtURI parsed;
  if (ParseAtURI(uri.asString(), parsed) != AtURIError::None) {
    return;
  }
  AtURIError error;
  std::string normalized = NormalizeAtURI(uri.asString(), error);
  if (error != AtURIError::None) {
    return;
  }
  aHtml += "<li><strong>";
  aHtml += EscapeHtml(aLabel);
  aHtml += "</strong>: <a href=\"";
  aHtml += EscapeHtml(normalized);
  aHtml += "\">";
  aHtml += EscapeHtml(normalized);
  aHtml += "</a></li>\n";
}

void AppendReferences(std::string& aHtml, const Json::Value& aValue) {
  std::string items;
  const Json::Value& reply = aValue["reply"];
  if (reply.isObject()) {
    AppendReference(items, "Parent", reply["parent"]);
    AppendReference(items, "Root", reply["root"]);
  }
  const Json::Value& embed = aValue["embed"];
  if (embed.isObject()) {
    AppendReference(items, "Quotes", embed["record"]);
    const Json::Value& media = embed["media"];
    if (media.isObject()) {
      AppendReference(items, "Quotes", media["record"]);
    }
  }
  if (items.empty()) {
    return;
  }
  aHtml += "<div class=\"refs\">References: <ul>\n";
  aHtml += items;
  aHtml += "</ul></div>\n";
}

std::string RenderPost(const Json::Value& aValue,
                       const LexiconViewProvenance& aProvenance) {
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
  AppendReferences(html, aValue);
  const Json::Value& embed = aValue["embed"];
  if (embed.isObject()) {
    AppendImages(html, embed["images"], aProvenance);
    const Json::Value& media = embed["media"];
    if (media.isObject()) {
      AppendImages(html, media["images"], aProvenance);
    }
  }
  html += "</section>\n";
  return html;
}

std::string RenderProfile(const Json::Value& aValue,
                          const LexiconViewProvenance& aProvenance) {
  std::string html;
  html += "<section class=\"lexicon\">\n<table>\n";
  AppendString(html, aValue, "displayName", "Name");
  AppendString(html, aValue, "description", "Description");
  html += "</table>\n";
  std::string blobs;
  AppendBlob(blobs, "Avatar", aValue["avatar"], Json::Value(), aProvenance);
  AppendBlob(blobs, "Banner", aValue["banner"], Json::Value(), aProvenance);
  if (!blobs.empty()) {
    html += "<div class=\"blobs\">Images: <ul>\n";
    html += blobs;
    html += "</ul></div>\n";
  }
  html += "</section>\n";
  return html;
}

}  // namespace

std::string RenderLexiconSummary(std::string_view aLexicon,
                                 std::string_view aValueJson,
                                 const LexiconViewProvenance& aProvenance) {
  if (aLexicon != "app.bsky.feed.post" &&
      aLexicon != "app.bsky.actor.profile") {
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

  return aLexicon == "app.bsky.feed.post" ? RenderPost(value, aProvenance)
                                          : RenderProfile(value, aProvenance);
}

}  // namespace mozilla::net::atproto
