# Packaging

Assets + script to build a self-contained **AppImage** of Tux-TI83 that runs
on any modern x86-64 Linux desktop with no system Qt install required.

| File | Purpose |
|---|---|
| `tux-ti83.desktop` | Desktop entry (name, icon, categories) |
| `tux-ti83.png` | 256×256 application icon |
| `build-appimage.sh` | One-shot: Release build → AppDir → AppImage |

## Build

```bash
./packaging/build-appimage.sh
```

Needs `linuxdeploy`, `linuxdeploy-plugin-qt`, and `appimagetool` on `PATH`
(download them from their GitHub releases), plus Qt 6 dev + CMake. Produces
`Tux-TI83-x86_64.AppImage` in the repo root.

## Run

```bash
chmod +x Tux-TI83-x86_64.AppImage
./Tux-TI83-x86_64.AppImage
```
