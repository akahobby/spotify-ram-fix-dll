# Spotify RAM Fix v0.9.2 (experimental)

This is a tuning update based on the **v0.9.1 source**, keeping `version.dll` proxy loading and per-process / experimental cross-process working-set trimming.

## Changes
- `MinWorkingSetMB=0` preserves unconditional trimming; set a positive threshold to skip needless trims.
- `EnableDebugLogging=0` avoids repeated DebugView messages during normal use; set to 1 while testing.
- `StartupDelayMs=5000` adjusts the first trim delay.
- Intervals are still bounded to 5 seconds–1 hour.

## Building
Upload all files, preserving `.github/workflows/build.yml`. GitHub Actions generates `build/Release/version.dll` and includes `settings.ini` in the artifact.

## Installing
Close Spotify fully. Back up any existing local `version.dll`. Put the newly compiled `version.dll` and `settings.ini` beside your website-installed `Spotify.exe` (usually `%APPDATA%\Spotify`). Do NOT overwrite the Windows System32 DLL. Remove only this custom DLL to revert.

## Notes
- **Not Windows compiled/tested here.** Cross-process trimming remains best effort and may fail in sandboxed processes.
- The lower displayed working-set number does **not** imply comparable reduction in private/committed memory; frequent trimming may cause page faults and UI latency.
- Keep MMCSS, raw-input and thread-priority hooks out of this build.
- This version intentionally does not try to inject into processes that do not load the proxy.
