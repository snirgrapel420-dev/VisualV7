#!/bin/bash
# =============================================================================
#  DALI VISUAL — macOS installer (.pkg)
#  Installs:
#    • AU          →  /Library/Audio/Plug-Ins/Components/Dali Visual.component
#    • VST3        →  /Library/Audio/Plug-Ins/VST3/Dali Visual.vst3
#    • Standalone  →  /Applications/Dali Visual.app
#  The user can untick any part ("Customize" in the installer).
#  Files installed by the macOS Installer are not quarantined, so the plug-ins load in the DAW straight away
#  (no Terminal / xattr step for the user).
#
#  Usage:  build_pkg.sh <artefacts dir (…/DaliVisual_artefacts/Release)> <version> <output dir>
#  Optional signing (Developer ID Installer certificate in the keychain):  PKG_SIGN_IDENTITY="Developer ID Installer: …"
# =============================================================================
set -euo pipefail

ART="$1"; VERSION="$2"; OUT="$3"
HERE="$(cd "$(dirname "$0")" && pwd)"
NAME="Dali Visual"
ID="com.daliaudio.dalivisual"
WORK="$(mktemp -d)"
mkdir -p "$OUT"

# component package for one bundle: staged in its own root, never relocated to a copy elsewhere on the disk
make_component () {
    local src="$1" dest="$2" suffix="$3" scripts="${4:-}"
    local root="$WORK/root-$suffix"
    mkdir -p "$root"
    cp -R "$src" "$root/"
    pkgbuild --analyze --root "$root" "$WORK/$suffix.plist"
    # the analysed plist is an array with one entry per bundle at the root
    plutil -replace 0.BundleIsRelocatable -bool NO "$WORK/$suffix.plist"
    plutil -replace 0.BundleOverwriteAction -string upgrade "$WORK/$suffix.plist" 2>/dev/null || true
    local extra=()
    if [ -n "$scripts" ]; then extra=(--scripts "$scripts"); fi
    pkgbuild --root "$root" \
             --component-plist "$WORK/$suffix.plist" \
             --install-location "$dest" \
             --identifier "$ID.$suffix" \
             --version "$VERSION" \
             ${extra[@]+"${extra[@]}"} \
             "$WORK/DaliVisual-$suffix.pkg"
}

chmod +x "$HERE/scripts-au/postinstall"
make_component "$ART/AU/$NAME.component"      "/Library/Audio/Plug-Ins/Components" au   "$HERE/scripts-au"
make_component "$ART/VST3/$NAME.vst3"         "/Library/Audio/Plug-Ins/VST3"       vst3
make_component "$ART/Standalone/$NAME.app"    "/Applications"                      app

sed "s/@VERSION@/$VERSION/g" "$HERE/distribution.xml" > "$WORK/distribution.xml"
sed "s/@VERSION@/$VERSION/g" "$HERE/resources/welcome.html" > "$WORK/welcome.html"
mkdir -p "$WORK/resources"
cp "$WORK/welcome.html" "$HERE/resources/conclusion.html" "$WORK/resources/"
# the DALI VISUAL logo in the bottom-left corner of the installer window
cp "$HERE/../../Resources/icon/icon_256.png" "$WORK/resources/logo.png"

PKG="$OUT/DaliVisual-$VERSION-macOS-Installer.pkg"
SIGN=()
if [ -n "${PKG_SIGN_IDENTITY:-}" ]; then SIGN=(--sign "$PKG_SIGN_IDENTITY"); fi
productbuild --distribution "$WORK/distribution.xml" \
             --resources "$WORK/resources" \
             --package-path "$WORK" \
             ${SIGN[@]+"${SIGN[@]}"} \
             "$PKG"

rm -rf "$WORK"
echo "Built: $PKG"
