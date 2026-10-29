# Spotify RAM Fix v0.8 — per-process working-set trim test

**Experimental, unverified on Windows.** Builds an x64 `version.dll` proxy using the v0.7 forwarding/export architecture. It creates one worker **in each Spotify process that loads this DLL**. Each worker trims only its own process. It does not hook priority, MMCSS, or raw input, and it does not touch the genuine System32 DLL.

## Build via GitHub Actions

Upload `version_proxy.cpp`, `version.def`, `CMakeLists.txt`, `settings.ini`, and `.github/workflows/build.yml` to the repository at the same paths. Run **Actions → Build Spotify RAM Fix DLL v0.8 → Run workflow**, then download the artifact.

## Test

1. Stop the old standalone optimizer and fully exit Spotify (including tray and its subprocesses).
2. Backup/remove any previous *custom* `%APPDATA%\Spotify\version.dll`. **Never touch** `%WINDIR%\System32\version.dll`.
3. Place the new `version.dll` and `settings.ini` beside `%APPDATA%\Spotify\Spotify.exe` (website-install build only).
4. Open DebugView as administrator; enable **Capture → Capture Win32**. Clear capture and restart Spotify.
5. Filter for `SpotifyRamFix v0.8` and look for `DLL_PROCESS_ATTACH`, `worker_started`, `trim_enabled`, and `trim ... success=1` (or `..._failed`). The first trim occurs around 5 seconds after the DLL loads and repeats every 30 seconds.
6. Test music, browsing, and game load for glitches. Expect only DLL-loading Spotify processes to be affected.

### Settings

`EnableTrim=1` enables trimming; `EnableTrim=0` leaves the worker inactive after startup logging. `TrimIntervalMs=30000` is the interval in milliseconds (clamped to 5000–3600000). Settings are read at startup; restart Spotify after changes.

### Limits and diagnostics

This intentionally uses `OutputDebugStringW` rather than file logging. Some low-integrity subprocess messages can be filtered or suppressed. `EmptyWorkingSet()` reduces the **resident working set**, not necessarily private committed bytes or actual application memory needs. Trimming can increase page faults and degrade responsiveness. Logging displays process ID, working set before/after, private committed bytes, success and Win32 error code.

Thread creation happens in `DllMain` but the worker performs no work until the loader lock is released; the DLL never waits inside `DllMain`. This is a compatibility experiment, not proof that this is a generally safe production initialization pattern. Remove the custom DLL if Spotify becomes unstable.

Note: Some Spotify subprocesses load the genuine Windows `version.dll`, so this DLL may not trim all processes. Avoid calling this a full seven-process replacement until measured.
