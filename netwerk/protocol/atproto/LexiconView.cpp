/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "LexiconView.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

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

constexpr size_t kMaxFacetUriLength = 2048;
constexpr size_t kMaxFacetTagLength = 100;

bool IsUtf8Continuation(unsigned char aByte) { return (aByte & 0xc0) == 0x80; }

size_t ClampToUtf8Boundary(std::string_view aText, uint64_t aOffset,
                           bool aForward) {
  size_t offset = static_cast<size_t>(
      std::min<uint64_t>(aOffset, static_cast<uint64_t>(aText.size())));
  if (aForward) {
    while (offset < aText.size() &&
           IsUtf8Continuation(static_cast<unsigned char>(aText[offset]))) {
      ++offset;
    }
    return offset;
  }
  while (offset < aText.size() &&
         IsUtf8Continuation(static_cast<unsigned char>(aText[offset]))) {
    --offset;
  }
  return offset;
}

bool StartsWithAsciiIgnoreCase(std::string_view aText,
                               std::string_view aLowerPrefix) {
  if (aText.size() < aLowerPrefix.size()) {
    return false;
  }
  for (size_t i = 0; i < aLowerPrefix.size(); ++i) {
    char c = aText[i];
    if (c >= 'A' && c <= 'Z') {
      c = static_cast<char>(c - 'A' + 'a');
    }
    if (c != aLowerPrefix[i]) {
      return false;
    }
  }
  return true;
}

bool HasUnsafeUrlByte(char aChar) {
  auto c = static_cast<unsigned char>(aChar);
  return c <= 0x20 || c == 0x7f || aChar == '"' || aChar == '\'' ||
         aChar == '<' || aChar == '>' || aChar == '\\' || aChar == '`';
}

bool IsSafeExternalLink(std::string_view aUrl) {
  if (aUrl.empty() || aUrl.size() > kMaxFacetUriLength) {
    return false;
  }
  for (char c : aUrl) {
    if (HasUnsafeUrlByte(c)) {
      return false;
    }
  }

  std::string_view scheme;
  if (StartsWithAsciiIgnoreCase(aUrl, "https://")) {
    scheme = "https://";
  } else if (StartsWithAsciiIgnoreCase(aUrl, "http://")) {
    scheme = "http://";
  } else {
    return false;
  }

  std::string_view rest = aUrl.substr(scheme.size());
  if (rest.empty()) {
    return false;
  }
  std::string_view authority = rest.substr(0, rest.find_first_of("/?#"));
  if (authority.empty() || authority.front() == '.' ||
      authority.back() == '.') {
    return false;
  }
  if (authority.find('@') != std::string_view::npos) {
    return false;
  }
  auto colon = authority.find(':');
  if (colon != std::string_view::npos) {
    if (authority.front() == '[') {
      if (authority.find(']') == std::string_view::npos) {
        return false;
      }
    } else {
      std::string_view port = authority.substr(colon + 1);
      if (port.empty() || port.find(':') != std::string_view::npos) {
        return false;
      }
      for (char c : port) {
        if (c < '0' || c > '9') {
          return false;
        }
      }
    }
  }
  return true;
}

bool IsSafeTag(std::string_view aTag) {
  if (aTag.empty() || aTag.size() > kMaxFacetTagLength) {
    return false;
  }
  for (unsigned char c : aTag) {
    if (c < 0x20 || c == 0x7f) {
      return false;
    }
  }
  return true;
}

std::string RenderFacet(std::string_view aText, const Json::Value& aFeature) {
  if (!aFeature.isObject()) {
    return {};
  }
  const Json::Value& type = aFeature["$type"];
  if (!type.isString()) {
    return {};
  }
  const std::string typeName = type.asString();

  if (typeName == "app.bsky.richtext.facet#link") {
    const Json::Value& uri = aFeature["uri"];
    if (!uri.isString() || !IsSafeExternalLink(uri.asString())) {
      return {};
    }
    return std::string("<a href=\"") + EscapeHtml(uri.asString()) +
           "\" rel=\"noopener noreferrer\">" + EscapeHtml(aText) + "</a>";
  }

  if (typeName == "app.bsky.richtext.facet#mention") {
    const Json::Value& did = aFeature["did"];
    if (!did.isString() || !IsValidDid(did.asString())) {
      return {};
    }
    AtURIError error;
    std::string uri = NormalizeAtURI("at://" + did.asString(), error);
    if (error != AtURIError::None) {
      return {};
    }
    return std::string("<a href=\"") + EscapeHtml(uri) + "\">" +
           EscapeHtml(aText) + "</a>";
  }

  if (typeName == "app.bsky.richtext.facet#tag") {
    const Json::Value& tag = aFeature["tag"];
    if (!tag.isString() || !IsSafeTag(tag.asString())) {
      return {};
    }
    return "<span class=\"hint\">" + EscapeHtml(aText) + "</span>";
  }

  return {};
}

struct FacetSegment {
  size_t mStart;
  size_t mEnd;
  std::string mHtml;
};

std::string RenderTextWithFacets(std::string_view aText,
                                 const Json::Value& aFacets) {
  std::vector<FacetSegment> segments;
  if (aFacets.isArray()) {
    for (const Json::Value& facet : aFacets) {
      if (!facet.isObject()) {
        continue;
      }
      const Json::Value& index = facet["index"];
      if (!index.isObject()) {
        continue;
      }
      const Json::Value& start = index["byteStart"];
      const Json::Value& end = index["byteEnd"];
      if (!start.isUInt64() || !end.isUInt64()) {
        continue;
      }
      uint64_t rawStart = start.asUInt64();
      uint64_t rawEnd = end.asUInt64();
      if (rawStart >= aText.size() || rawEnd > aText.size() ||
          rawStart >= rawEnd) {
        continue;
      }
      size_t byteStart = ClampToUtf8Boundary(aText, rawStart, true);
      size_t byteEnd = ClampToUtf8Boundary(aText, rawEnd, false);
      if (byteStart >= byteEnd) {
        continue;
      }
      std::string html = RenderFacet(
          aText.substr(byteStart, byteEnd - byteStart), facet["feature"]);
      if (!html.empty()) {
        segments.push_back({byteStart, byteEnd, std::move(html)});
      }
    }
  }

  std::sort(segments.begin(), segments.end(),
            [](const FacetSegment& aLeft, const FacetSegment& aRight) {
              if (aLeft.mStart != aRight.mStart) {
                return aLeft.mStart < aRight.mStart;
              }
              return aLeft.mEnd < aRight.mEnd;
            });

  std::string out;
  size_t cursor = 0;
  for (const FacetSegment& segment : segments) {
    if (segment.mStart < cursor) {
      continue;
    }
    out += EscapeHtml(aText.substr(cursor, segment.mStart - cursor));
    out += segment.mHtml;
    cursor = segment.mEnd;
  }
  out += EscapeHtml(aText.substr(cursor));
  return out;
}

void AppendPostText(std::string& aHtml, const Json::Value& aValue) {
  const Json::Value& field = aValue["text"];
  if (!field.isString() || field.asString().empty()) {
    return;
  }
  const std::string text = field.asString();
  aHtml += "<tr><th>Text</th><td>";
  aHtml += RenderTextWithFacets(text, aValue["facets"]);
  aHtml += "</td></tr>\n";
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

void AppendDidLink(std::string& aHtml, std::string_view aLabel,
                   const Json::Value& aDid) {
  if (!aDid.isString() || !IsValidDid(aDid.asString())) {
    return;
  }
  std::string uri = "at://" + aDid.asString();
  aHtml += "<li><strong>";
  aHtml += EscapeHtml(aLabel);
  aHtml += "</strong>: <a href=\"";
  aHtml += EscapeHtml(uri);
  aHtml += "\">";
  aHtml += EscapeHtml(uri);
  aHtml += "</a></li>\n";
}

std::string RenderSocialAction(const Json::Value& aValue,
                               std::string_view aLexicon) {
  const char* label = aLexicon == "app.bsky.feed.like"     ? "Liked"
                      : aLexicon == "app.bsky.feed.repost" ? "Reposted"
                                                           : "Follows";
  std::string html;
  html += "<section class=\"lexicon\">\n<table>\n";
  AppendString(html, aValue, "createdAt", "Date");
  html += "</table>\n";

  std::string items;
  if (aLexicon == "app.bsky.graph.follow") {
    AppendDidLink(items, label, aValue["subject"]);
  } else {
    AppendReference(items, label, aValue["subject"]);
  }
  if (!items.empty()) {
    html += "<div class=\"refs\"><ul>\n";
    html += items;
    html += "</ul></div>\n";
  }
  html += "</section>\n";
  return html;
}

std::string RenderPost(const Json::Value& aValue,
                       const LexiconViewProvenance& aProvenance) {
  std::string html;
  html += "<section class=\"lexicon\">\n<table>\n";
  AppendPostText(html, aValue);
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
      aLexicon != "app.bsky.actor.profile" &&
      aLexicon != "app.bsky.feed.like" && aLexicon != "app.bsky.feed.repost" &&
      aLexicon != "app.bsky.graph.follow") {
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

  if (aLexicon == "app.bsky.feed.post") {
    return RenderPost(value, aProvenance);
  }
  if (aLexicon == "app.bsky.actor.profile") {
    return RenderProfile(value, aProvenance);
  }
  return RenderSocialAction(value, aLexicon);
}

}  // namespace mozilla::net::atproto
