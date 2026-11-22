/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ErrorView.h"

#include <string_view>

#include "AtDocumentPolicy.h"

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

void AppendRow(std::string& aHtml, std::string_view aLabel,
               const std::string& aValue) {
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

const char* FailureTitle(FailureKind aKind) {
  switch (aKind) {
    case FailureKind::HandleResolutionFailed:
      return "Handle could not be resolved";
    case FailureKind::HandleBindingFailed:
      return "Handle does not match its DID";
    case FailureKind::DidResolutionFailed:
      return "DID could not be resolved";
    case FailureKind::DidDocumentRejected:
      return "DID document was rejected";
    case FailureKind::OnionEndpointUnsupported:
      return "PDS is onion-only; native Tor support is not available";
    case FailureKind::PdsUnreachable:
      return "Repository server could not be reached";
    case FailureKind::RecordNotFound:
      return "Record not found";
    case FailureKind::RecordInvalid:
      return "Record could not be read";
    case FailureKind::RepoNotFound:
      return "Repository not found";
    case FailureKind::RepoInvalid:
      return "Repository description could not be read";
    case FailureKind::CollectionNotFound:
      return "Collection not found";
    case FailureKind::CollectionInvalid:
      return "Collection listing could not be read";
    case FailureKind::UpstreamRejected:
      return "Server rejected the request";
    case FailureKind::RedirectRejected:
      return "Redirect left the expected origin";
    case FailureKind::NetworkError:
      return "Network error";
  }
  return "Record could not be loaded";
}

const char* FailureExplanation(FailureKind aKind) {
  switch (aKind) {
    case FailureKind::HandleResolutionFailed:
      return "The handle could not be resolved to a DID through its DNS TXT "
             "record or its HTTPS well-known endpoint.";
    case FailureKind::HandleBindingFailed:
      return "The resolved DID document does not claim this handle, so the "
             "binding was rejected.";
    case FailureKind::DidResolutionFailed:
      return "The DID document could not be fetched from its identity host.";
    case FailureKind::DidDocumentRejected:
      return "The DID document is malformed or names a repository endpoint "
             "raindrop will not contact.";
    case FailureKind::OnionEndpointUnsupported:
      return "The DID document names an onion repository server. Raindrop has "
             "no native Tor transport and will not proxy the request through a "
             "gateway, so this record cannot be reached.";
    case FailureKind::PdsUnreachable:
      return "The repository server named by the DID document did not answer.";
    case FailureKind::RecordNotFound:
      return "The repository server reports that this record does not exist "
             "at the requested collection and key.";
    case FailureKind::RecordInvalid:
      return "The repository server returned a response that could not be "
             "parsed as a record.";
    case FailureKind::RepoNotFound:
      return "The repository server reports that no repository exists for "
             "this DID.";
    case FailureKind::RepoInvalid:
      return "The repository server returned a response that could not be "
             "parsed as a repository description.";
    case FailureKind::CollectionNotFound:
      return "The repository server reports that this repository has no such "
             "collection.";
    case FailureKind::CollectionInvalid:
      return "The repository server returned a response that could not be "
             "parsed as a collection listing.";
    case FailureKind::UpstreamRejected:
      return "An upstream server returned a failure status; no gateway was "
             "tried.";
    case FailureKind::RedirectRejected:
      return "An upstream response redirected to a different origin — a "
             "different host, scheme or port — which is not followed for "
             "identity-sensitive requests.";
    case FailureKind::NetworkError:
      return "The request failed before a response could be read.";
  }
  return "The record could not be loaded.";
}

std::string RenderFailureHtml(const FailureInfo& aInfo,
                              const ViewStyleOptions& aStyle) {
  std::string html;
  html.reserve(1024);
  html += "<!doctype html><html><head><meta charset=\"utf-8\">\n";
  AppendDocumentSecurityMeta(html);
  html += "<title>Raindrop — ";
  html += EscapeHtml(FailureTitle(aInfo.mKind));
  html += "</title>\n";
  AppendDocumentStyle(html, aStyle);
  html += "</head><body>\n";
  html += "<h1>";
  html += EscapeHtml(FailureTitle(aInfo.mKind));
  html += "</h1>\n<p>";
  html += EscapeHtml(FailureExplanation(aInfo.mKind));
  html += "</p>\n";

  html += "<table>\n";
  AppendRow(html, "Requested", aInfo.mAuthority);
  AppendRow(html, "Resolved DID", aInfo.mDid);
  AppendRow(html, "Repository server", aInfo.mPds);
  AppendRow(html, "Detail", aInfo.mDetail);
  html += "</table>\n";

  html +=
      "<p class=\"hint\">Raindrop resolves records directly and does not "
      "fall back to a gateway. Nothing was cached or sent to a third "
      "party to repair this lookup.</p>\n";
  html += "</body></html>\n";
  return html;
}

}  // namespace mozilla::net::atproto
