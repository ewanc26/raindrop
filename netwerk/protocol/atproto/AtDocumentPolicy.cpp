/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "AtDocumentPolicy.h"

#include "mozilla/Preferences.h"
#include "nsString.h"

namespace mozilla::net::atproto {

const char kAtDocumentCsp[] =
    "default-src 'none'; script-src 'none'; style-src 'unsafe-inline'; "
    "img-src https:; object-src 'none'; base-uri 'none'; form-action 'none'; "
    "frame-ancestors 'none'";

namespace {

constexpr const char kLightVars[] =
    "--rd-bg:#ffffff;--rd-fg:#222222;--rd-muted:#555555;"
    "--rd-border:#cccccc;--rd-th-bg:#f4f4f4;--rd-lexicon-bg:#fafafa";
constexpr const char kDarkVars[] =
    "--rd-bg:#181a1b;--rd-fg:#eeeeee;--rd-muted:#aaaaaa;"
    "--rd-border:#444444;--rd-th-bg:#242628;--rd-lexicon-bg:#242628";
constexpr const char kContrastVars[] =
    "--rd-bg:#000000;--rd-fg:#ffffff;--rd-muted:#ffffff;"
    "--rd-border:#ffffff;--rd-th-bg:#000000;--rd-lexicon-bg:#000000";

struct AccentPalette {
  const char* mLight;
  const char* mDark;
};

AccentPalette SelectAccent(ViewAccent aAccent) {
  switch (aAccent) {
    case ViewAccent::Green:
      return {"#2f7d4f", "#81c995"};
    case ViewAccent::Purple:
      return {"#7057a8", "#c58af9"};
    case ViewAccent::Amber:
      return {"#a1651d", "#fdd663"};
    case ViewAccent::Slate:
      return {"#4a5b6a", "#9aa0a6"};
    case ViewAccent::Blue:
      break;
  }
  return {"#3b6ea5", "#8ab4f8"};
}

nsCString ReadPref(const char* aName, const char* aFallback) {
  nsAutoCString value;
  if (NS_FAILED(Preferences::GetCString(aName, value)) || value.IsEmpty()) {
    return nsCString(aFallback);
  }
  return nsCString(value);
}

void AppendVar(std::string& aHtml, const char* aName, const char* aValue) {
  aHtml += "--rd-";
  aHtml += aName;
  aHtml += ":";
  aHtml += aValue;
  aHtml += ";";
}

void AppendAccent(std::string& aHtml, const char* aColor) {
  aHtml += ":root{";
  AppendVar(aHtml, "link", aColor);
  AppendVar(aHtml, "accent", aColor);
  aHtml += "}\n";
}

}  // namespace

ViewStyleOptions LoadViewStyleOptions() {
  ViewStyleOptions options;

  nsCString theme = ReadPref("network.atprotocol.view.theme", "system");
  if (theme == "light") {
    options.mTheme = ViewTheme::Light;
  } else if (theme == "dark") {
    options.mTheme = ViewTheme::Dark;
  } else if (theme == "high-contrast") {
    options.mTheme = ViewTheme::HighContrast;
  }

  nsCString textSize = ReadPref("network.atprotocol.view.textSize", "medium");
  if (textSize == "small") {
    options.mTextSize = ViewTextSize::Small;
  } else if (textSize == "large") {
    options.mTextSize = ViewTextSize::Large;
  } else if (textSize == "extra-large") {
    options.mTextSize = ViewTextSize::ExtraLarge;
  }

  nsCString width = ReadPref("network.atprotocol.view.width", "standard");
  if (width == "narrow") {
    options.mWidth = ViewWidth::Narrow;
  } else if (width == "wide") {
    options.mWidth = ViewWidth::Wide;
  } else if (width == "full") {
    options.mWidth = ViewWidth::Full;
  }

  nsCString accent = ReadPref("network.atprotocol.view.accent", "blue");
  if (accent == "green") {
    options.mAccent = ViewAccent::Green;
  } else if (accent == "purple") {
    options.mAccent = ViewAccent::Purple;
  } else if (accent == "amber") {
    options.mAccent = ViewAccent::Amber;
  } else if (accent == "slate") {
    options.mAccent = ViewAccent::Slate;
  }

  options.mCompact =
      ReadPref("network.atprotocol.view.density", "comfortable") == "compact";
  return options;
}

void AppendDocumentSecurityMeta(std::string& aHtml) {
  aHtml += "<meta http-equiv=\"Content-Security-Policy\" content=\"";
  aHtml += kAtDocumentCsp;
  aHtml += "\">\n<meta name=\"referrer\" content=\"no-referrer\">\n";
}

void AppendDocumentStyle(std::string& aHtml, const ViewStyleOptions& aOptions) {
  aHtml += "<style>\n";
  aHtml += ":root{color-scheme:light dark;";
  aHtml += kLightVars;
  aHtml += "}\n@media(prefers-color-scheme:dark){:root{";
  aHtml += kDarkVars;
  aHtml += "}}\n";

  AccentPalette accent = SelectAccent(aOptions.mAccent);
  if (aOptions.mTheme == ViewTheme::Light) {
    AppendAccent(aHtml, accent.mLight);
  } else if (aOptions.mTheme == ViewTheme::Dark) {
    AppendAccent(aHtml, accent.mDark);
  } else if (aOptions.mTheme != ViewTheme::HighContrast) {
    aHtml += ":root{";
    AppendVar(aHtml, "link", accent.mLight);
    AppendVar(aHtml, "accent", accent.mLight);
    aHtml += "}\n@media(prefers-color-scheme:dark){:root{";
    AppendVar(aHtml, "link", accent.mDark);
    AppendVar(aHtml, "accent", accent.mDark);
    aHtml += "}}\n";
  }

  aHtml += ":root{";
  switch (aOptions.mTextSize) {
    case ViewTextSize::Small:
      AppendVar(aHtml, "font-size", "12px");
      break;
    case ViewTextSize::Large:
      AppendVar(aHtml, "font-size", "17px");
      break;
    case ViewTextSize::ExtraLarge:
      AppendVar(aHtml, "font-size", "20px");
      break;
    case ViewTextSize::Medium:
      AppendVar(aHtml, "font-size", "14px");
      break;
  }
  switch (aOptions.mWidth) {
    case ViewWidth::Narrow:
      AppendVar(aHtml, "max-width", "38em");
      break;
    case ViewWidth::Wide:
      AppendVar(aHtml, "max-width", "82em");
      break;
    case ViewWidth::Full:
      AppendVar(aHtml, "max-width", "none");
      break;
    case ViewWidth::Standard:
      AppendVar(aHtml, "max-width", "60em");
      break;
  }
  AppendVar(aHtml, "margin", aOptions.mCompact ? "1em" : "2em");
  AppendVar(aHtml, "cell-padding",
            aOptions.mCompact ? ".2em .4em" : ".3em .6em");
  aHtml += "}\n";

  switch (aOptions.mTheme) {
    case ViewTheme::Light:
      aHtml += ":root{color-scheme:light;";
      aHtml += kLightVars;
      aHtml += "}\n";
      break;
    case ViewTheme::Dark:
      aHtml += ":root{color-scheme:dark;";
      aHtml += kDarkVars;
      aHtml += "}\n";
      break;
    case ViewTheme::HighContrast:
      aHtml += ":root{color-scheme:light dark;";
      aHtml += kContrastVars;
      AppendVar(aHtml, "link", "#ffff00");
      AppendVar(aHtml, "accent", "#ffff00");
      aHtml += "}a{text-decoration:underline}\n";
      break;
    case ViewTheme::System:
      break;
  }

  aHtml += "html{background:var(--rd-bg);color:var(--rd-fg)}\n";
  aHtml +=
      "body{font:var(--rd-font-size) system-ui,sans-serif;line-height:1.5;"
      "margin:var(--rd-margin);max-width:var(--rd-max-width);"
      "background:var(--rd-bg);color:var(--rd-fg)}\n";
  aHtml += "a{color:var(--rd-link)}\n";
  aHtml += "table{border-collapse:collapse;margin-bottom:1em}\n";
  aHtml +=
      "th,td{border:1px solid var(--rd-border);padding:var(--rd-cell-padding);"
      "text-align:left;vertical-align:top}\n";
  aHtml += "th{background:var(--rd-th-bg);white-space:nowrap}\n";
  aHtml += "code{word-break:break-all}\n";
  aHtml += "pre{white-space:pre-wrap;word-break:break-word}\n";
  aHtml += "li{margin-bottom:.4em}\n";
  aHtml += ".hint{color:var(--rd-muted);font-size:.9em}\n";
  aHtml += ".langs{color:var(--rd-muted);font-size:.9em}\n";
  aHtml +=
      ".lexicon{margin:1em 0;padding:.5em 1em;"
      "background:var(--rd-lexicon-bg);"
      "border-left:3px solid var(--rd-accent)}\n";
  aHtml +=
      ".blob{max-width:100%;max-height:32em;display:block;margin:.4em 0}\n";
  aHtml += "</style>\n";
}

}  // namespace mozilla::net::atproto
