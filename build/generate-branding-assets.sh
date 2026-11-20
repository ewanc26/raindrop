#!/usr/bin/env bash
# Regenerate every raster/platform asset in browser/branding/raindrop from the
# original SVG artwork in browser/branding/raindrop/source/.
#
# Requirements: rsvg-convert and ImageMagick (brew install librsvg imagemagick),
# plus Apple's iconutil/actool for the macOS assets.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BRAND="$ROOT/browser/branding/raindrop"
SRC="$BRAND/source"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

rsvg() { rsvg-convert "$@"; }
png()  { rsvg -w "$2" -h "$2" "$1" -o "$3"; }

echo "Generating PNG icons..."
for size in 16 22 24 32 48 64 128 256; do
  png "$SRC/icon.svg" "$size" "$BRAND/default${size}.png"
done

echo "Generating about-screen assets..."
png "$SRC/glyph.svg" 192 "$BRAND/content/about-logo.png"
png "$SRC/glyph.svg" 384 "$BRAND/content/about-logo@2x.png"
png "$SRC/glyph-private.svg" 192 "$BRAND/content/about-logo-private.png"
png "$SRC/glyph-private.svg" 384 "$BRAND/content/about-logo-private@2x.png"
cp "$SRC/glyph.svg" "$BRAND/content/about-logo.svg"
cp "$SRC/wordmark.svg" "$BRAND/content/about-wordmark.svg"
cp "$SRC/wordmark.svg" "$BRAND/content/firefox-wordmark.svg"
cp "$SRC/document-pdf.svg" "$BRAND/content/document_pdf.svg"

magick -size 300x236 "gradient:#0b1b3a-#0a2540" "$TMP/about-bg.png"
rsvg -w 104 -h 104 "$SRC/glyph.svg" -o "$TMP/glyph104.png"
rsvg -w 140 -h 104 "$SRC/wordmark.svg" -o "$TMP/wordmark-full.png"
magick "$TMP/wordmark-full.png" -resize 140x36\! "$TMP/wordmark140.png"
magick "$TMP/about-bg.png" "$TMP/glyph104.png" -gravity northwest \
  -geometry +80+38 -composite "$TMP/wordmark140.png" -geometry +80+150 \
  -composite "PNG24:$BRAND/content/about.png"

echo "Generating Windows icons..."
magick "$BRAND/default256.png" -define icon:auto-resize=256,128,64,48,32,16 "$BRAND/firefox.ico"
png "$SRC/icon.svg" 64 "$TMP/icon64.png"
magick "$TMP/icon64.png" "$BRAND/firefox64.ico"
png "$SRC/icon-private.svg" 256 "$TMP/pb256.png"
magick "$TMP/pb256.png" -define icon:auto-resize=256,128,64,48,32,16 "$BRAND/pbmode.ico"
for name in newtab newwindow; do
  magick "$BRAND/firefox.ico" "$BRAND/${name}.ico"
done
png "$SRC/document.svg" 256 "$TMP/document.png"
magick "$TMP/document.png" -define icon:auto-resize=256,128,64,48,32,16 "$BRAND/document.ico"
magick "$TMP/document.png" -define icon:auto-resize=256,128,64,48,32,16 \
  "$BRAND/content/document_pdf.ico" 2>/dev/null || true
png "$SRC/document-pdf.svg" 256 "$TMP/document-pdf.png"
magick "$TMP/document-pdf.png" -define icon:auto-resize=256,128,64,48,32,16 \
  "$BRAND/content/document_pdf.ico"

echo "Generating macOS icons..."
ICONSET="$TMP/raindrop.iconset"
mkdir -p "$ICONSET"
for pair in "16:icon_16x16.png" "32:icon_16x16@2x.png" "32:icon_32x32.png" \
            "64:icon_32x32@2x.png" "128:icon_128x128.png" "256:icon_128x128@2x.png" \
            "256:icon_256x256.png" "512:icon_256x256@2x.png" "512:icon_512x512.png" \
            "1024:icon_512x512@2x.png"; do
  size="${pair%%:*}"
  name="${pair#*:}"
  png "$SRC/icon.svg" "$size" "$ICONSET/$name"
done
iconutil -c icns "$ICONSET" -o "$BRAND/firefox.icns"
cp "$BRAND/firefox.icns" "$BRAND/disk.icns"
png "$SRC/document.svg" 512 "$TMP/document512.png"
magick "$TMP/document512.png" -define icon:auto-resize=256,128,64,32,16 \
  "$BRAND/document.icns" 2>/dev/null || true
# iconutil is the reliable path to a multi-resolution .icns.
DOCSET="$TMP/document.iconset"
mkdir -p "$DOCSET"
for size in 16 32 128 256 512 1024; do
  png "$SRC/document.svg" "$size" "$DOCSET/icon_${size}x${size}.png"
done
iconutil -c icns "$DOCSET" -o "$BRAND/document.icns"

echo "Generating Assets.car (macOS asset catalog)..."
CATALOG="$TMP/Raindrop.xcassets/AppIcon.appiconset"
mkdir -p "$CATALOG"
cat > "$CATALOG/Contents.json" <<'JSON'
{
  "images": [
    { "filename": "icon_16.png", "idiom": "mac", "scale": "1x", "size": "16x16" },
    { "filename": "icon_16@2x.png", "idiom": "mac", "scale": "2x", "size": "16x16" },
    { "filename": "icon_32.png", "idiom": "mac", "scale": "1x", "size": "32x32" },
    { "filename": "icon_32@2x.png", "idiom": "mac", "scale": "2x", "size": "32x32" },
    { "filename": "icon_128.png", "idiom": "mac", "scale": "1x", "size": "128x128" },
    { "filename": "icon_128@2x.png", "idiom": "mac", "scale": "2x", "size": "128x128" },
    { "filename": "icon_256.png", "idiom": "mac", "scale": "1x", "size": "256x256" },
    { "filename": "icon_256@2x.png", "idiom": "mac", "scale": "2x", "size": "256x256" },
    { "filename": "icon_512.png", "idiom": "mac", "scale": "1x", "size": "512x512" },
    { "filename": "icon_512@2x.png", "idiom": "mac", "scale": "2x", "size": "512x512" }
  ],
  "info": { "author": "raindrop", "version": 1 }
}
JSON
cp "$ICONSET/icon_16x16.png" "$CATALOG/icon_16.png"
cp "$ICONSET/icon_16x16@2x.png" "$CATALOG/icon_16@2x.png"
cp "$ICONSET/icon_32x32.png" "$CATALOG/icon_32.png"
cp "$ICONSET/icon_32x32@2x.png" "$CATALOG/icon_32@2x.png"
cp "$ICONSET/icon_128x128.png" "$CATALOG/icon_128.png"
cp "$ICONSET/icon_128x128@2x.png" "$CATALOG/icon_128@2x.png"
cp "$ICONSET/icon_256x256.png" "$CATALOG/icon_256.png"
cp "$ICONSET/icon_256x256@2x.png" "$CATALOG/icon_256@2x.png"
cp "$ICONSET/icon_512x512.png" "$CATALOG/icon_512.png"
cp "$ICONSET/icon_512x512@2x.png" "$CATALOG/icon_512@2x.png"
mkdir -p "$TMP/car-out"
xcrun actool --compile "$TMP/car-out" \
  --app-icon AppIcon \
  --output-partial-info-plist "$TMP/partial.plist" \
  --platform macosx --minimum-deployment-target 11.0 \
  "$TMP/Raindrop.xcassets" >/dev/null
cp "$TMP/car-out/Assets.car" "$BRAND/Assets.car"

echo "Generating installer artwork..."
magick -size 1440x880 "gradient:#0b1b3a-#061426" "$TMP/dmg-bg.png"
png "$SRC/glyph.svg" 300 "$TMP/glyph300.png"
png "$SRC/wordmark.svg" 420 "$TMP/wordmark420.png"
magick "$TMP/dmg-bg.png" "$TMP/glyph300.png" -gravity center \
  -geometry +0-70 -composite "$TMP/wordmark420.png" -geometry +0+170 \
  -composite "PNG24:$BRAND/background.png"
png "$SRC/icon.svg" 150 "$BRAND/VisualElements_150.png"
png "$SRC/icon.svg" 70 "$BRAND/VisualElements_70.png"
png "$SRC/icon-private.svg" 150 "$BRAND/PrivateBrowsing_150.png"
png "$SRC/icon-private.svg" 70 "$BRAND/PrivateBrowsing_70.png"
magick "$BRAND/default128.png" -flip "$BRAND/wizHeaderRTL.bmp"
magick "$BRAND/default128.png" -resize 100x60\! "$BRAND/wizHeader.bmp"
magick -size 60x60 "gradient:#0b1b3a-#061426" "$BRAND/wizWatermark.bmp"
magick -size 600x400 "gradient:#0b1b3a-#22d3ee" -quality 85 \
  "$BRAND/stubinstaller/bgstub.jpg"

echo "Generating MSIX artwork..."
MSIX="$BRAND/msix/Assets"
mkdir -p "$MSIX"
while IFS='|' read -r file size; do
  png "$SRC/icon.svg" "$size" "$MSIX/$file"
done <<'LIST'
Document44x44.png|44
LargeTile.scale-200.png|620
MedTile.scale-200.png|300
SmallTile.scale-200.png|142
Square150x150Logo.scale-200.png|300
Square44x44Logo.altform-lightunplated_targetsize-256.png|256
Square44x44Logo.altform-unplated_targetsize-256.png|256
Square44x44Logo.scale-200.png|88
Square44x44Logo.targetsize-256.png|256
StoreLogo.scale-200.png|100
LIST
magick -size 620x300 "gradient:#0b1b3a-#061426" "$TMP/wide.png"
png "$SRC/icon.svg" 280 "$TMP/wide-icon.png"
magick "$TMP/wide.png" "$TMP/wide-icon.png" -gravity center -composite \
  "PNG24:$MSIX/Wide310x150Logo.scale-200.png"

echo "Done. Generated assets in $BRAND"
