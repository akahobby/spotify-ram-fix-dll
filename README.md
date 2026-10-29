# Spotify RAM Fix v1.0

Native Windows x64 `version.dll` proxy for the **Spotify desktop installer (website version)**. Builds on the user-tested v0.9.3 coordinator release. This is an **experimental release candidate**, not an independently validated stable build.

## What it does

- For Spotify subprocesses that load the application-local `version.dll`, starts a periodic working-set trim using Windows `EmptyWorkingSet`.
- Optional single cross-process coordinator *attempts* to trim sibling `Spotify.exe` processes with the same executable path. Sandbox permissions can block this.
- Preserves standard Version API exports by forwarding them to the system `version.dll`.
- Leaves MMCSS, audio-thread priorities and raw input unchanged.

**Important:** Trimming removes resident pages from a process's working set. It does **not** necessarily free committed private memory or fix a leak. A low Task Manager figure may recover quickly and page faults can increase. Some Spotify processes use the system DLL, so not all seven will necessarily be trimmed.

## Build

On GitHub: Actions > **Build Spotify RAM Fix v1.0** > Run workflow. Download and extract the `SpotifyRamFix-v1.0-Windows-x64` artifact, then extract the ZIP inside. This repository ZIP is **source**, not a prebuilt DLL.

Alternatively, on Windows with Visual Studio C++ and CMake:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

Copy `build/Release/version.dll` beside `settings.ini`, `install.ps1` and `uninstall.ps1` before installing.

## Install / update

1. Fully exit Spotify, including the tray icon. Stop any older standalone RAM optimizer to avoid confounding results.
2. Back up your current known-good v0.9.3 `version.dll` and settings separately.
3. From the **compiled release directory**, run `install.ps1` in PowerShell (e.g. `powershell -ExecutionPolicy Bypass -File .\install.ps1`). Only run scripts you've reviewed and trust.
4. The script checks for the **website** Spotify installation at `%APPDATA%\Spotify`, backs up any existing `version.dll` and `settings.ini` before installing, and writes an install record.
5. Start Spotify and verify playback and memory behavior. If an installation record already exists, run its corresponding uninstall first.

## Uninstall / rollback

Exit Spotify, run `uninstall.ps1` from the release directory, and restart Spotify. It restores backed-up files when hashes still match; it refuses to delete a DLL modified after installation. **If installed manually, remove only the custom DLL you installed, never Windows' System32 DLL.**

## Configuration

`settings.ini` is alongside the DLL. Defaults: 30 s trim interval, 5 s startup delay, zero working-set threshold, coordinator enabled, verbose debug output off. Disable cross-process attempts with `EnableCrossProcessTrim=0` if access denied logs are noisy or if you want self-only behavior. Set `EnableDebugLogging=1` to inspect output in DebugView (Capture Win32). Restart Spotify after changing settings.

## Validation checklist

Run an A/B comparison over comparable Spotify usage (same playback/idle conditions), before and after installation. Record total working set, private commit, page faults, CPU usage, UI navigation responsiveness and playback under load. Observe for at least 1–2 hours. Do not interpret working-set reductions as equivalent to freed application allocations.

## Compatibility and cautions

- Windows x64 only; application-local DLL loading is not guaranteed on other Spotify versions.
- Third-party, unofficial modification; a Spotify update could break loading, and replacing app-local DLLs could disrupt Spotify.
- This release does not bypass sandbox permissions or guarantee seven-process coverage.
- Source is provided for inspection; no binaries are bundled in the source ZIP.

## License

MIT — see LICENSE. This project's code is separate from the third-party Discord fix discussed during development.
