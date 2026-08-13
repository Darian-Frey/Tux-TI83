#!/usr/bin/env bash
# Build a self-contained AppImage of Tux-TI83.
#
# Prerequisites (must be on PATH):
#   - linuxdeploy               (https://github.com/linuxdeploy/linuxdeploy)
#   - linuxdeploy-plugin-qt     (…/linuxdeploy-plugin-qt) — bundles Qt + QML
#   - appimagetool              (https://github.com/AppImage/appimagetool)
#   - Qt 6 dev + qmake6, CMake, a C++20 compiler
#
# Output: ./Tux-TI83-x86_64.AppImage (in the repo root)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

echo "==> Release build"
cmake -S . -B build-appimage -DCMAKE_BUILD_TYPE=Release
cmake --build build-appimage --target tux_ti83 -j"$(nproc)"

echo "==> Staging AppDir"
rm -rf AppDir
mkdir -p AppDir/usr/bin \
         AppDir/usr/share/applications \
         AppDir/usr/share/icons/hicolor/256x256/apps
cp build-appimage/tux_ti83                 AppDir/usr/bin/
cp packaging/tux-ti83.desktop              AppDir/usr/share/applications/
cp packaging/tux-ti83.png                  AppDir/usr/share/icons/hicolor/256x256/apps/

echo "==> Bundling Qt + QML and building the AppImage"
# QML_SOURCES_PATHS lets the qt plugin discover the app's QML imports
# (QtQuick, QtQuick.Controls, QtQuick.Layouts) and bundle those modules.
export QML_SOURCES_PATHS="$ROOT/app/qml"
export QMAKE="$(command -v qmake6 || command -v qmake)"
export EXTRA_QT_MODULES="quick;qml"
export APPIMAGE_EXTRACT_AND_RUN=1
export OUTPUT="Tux-TI83-x86_64.AppImage"

linuxdeploy --appdir AppDir --plugin qt --output appimage \
  -d packaging/tux-ti83.desktop -i packaging/tux-ti83.png

echo "==> Done: $ROOT/Tux-TI83-x86_64.AppImage"
