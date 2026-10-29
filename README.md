# Spotify Version DLL Probe — corrected build (v0.1.1)

This is a **diagnostic DLL only**. It does not optimize memory. It attempts to log its loading to `%LOCALAPPDATA%\SpotifyVersionProbe.log` and forwards 16 version API exports to the genuine Windows `version.dll`.

## Fix
The original build declared export functions using names/signatures that conflicted with `winver.h`. This edition defines uniquely named wrapper functions (`Proxy_*`) and maps the Windows API exports with `version.def`. It removes the conflicting `winver.h` include.

## GitHub Actions
Replace the files at the repository root (especially `version_probe.cpp`, `CMakeLists.txt`) **and add `version.def`**. Preserve `.github/workflows/build.yml`. Run **Actions → Build Windows DLL probe → Run workflow**. Download `SpotifyVersionProbe-x64` if the build succeeds.

## Test and safety
Close Spotify, make sure you have the website desktop build, and check that no existing `version.dll` is in the Spotify executable directory before copying the compiled test DLL there. Never overwrite the Windows system DLL or a preexisting app DLL. Restart Spotify, then examine `%LOCALAPPDATA%\SpotifyVersionProbe.log` and Procmon's **Load Image** results for the actual loaded path. On failure to launch, remove only the test DLL. Remove the test DLL after diagnosis.

A successful load is **not guaranteed**: a System32 reference in Procmon does not prove app-local DLL lookup. No Windows execution testing has been performed here.
