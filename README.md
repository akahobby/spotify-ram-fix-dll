# Spotify RAM Fix v0.7 — minimal DLL initialization probe

**Test-only. Does not reduce RAM.** No workers, mutexes, trimming, filesystem writes or user configuration.

## What it tests

- `DllMain(DLL_PROCESS_ATTACH)` emits `SpotifyRamFix v0.7: DLL_PROCESS_ATTACH` through `OutputDebugStringW`.
- If Spotify invokes a Version API export from the proxy, the proxy emits a second `SpotifyRamFix v0.7: version API export called` debug message, and forwards that call to Windows' genuine System32 `version.dll`.
- Loading the DLL in Procmon alone proves mapping, not that the debug listener captured the message.

## GitHub Actions build

Upload `version_proxy.cpp`, `version.def`, `CMakeLists.txt`, and `.github/workflows/build.yml` to the repository root (creating `.github/workflows/build.yml` through the GitHub web editor if hidden locally). In Actions, run **Build Spotify DLL Probe v0.7** and download the artifact.

## Test safely

1. Fully exit Spotify and any old memory optimizer EXE.
2. Back up/remove the older custom `%APPDATA%\Spotify\version.dll`. **Never replace System32's version.dll.**
3. Put the new probe `version.dll` next to `%APPDATA%\Spotify\Spotify.exe`.
4. Start **DebugView as Administrator**; enable Capture > Capture Win32; clear the view and relaunch Spotify.
5. Search for `SpotifyRamFix v0.7` in DebugView; also use Procmon to verify `Load Image` of the exact AppData DLL path.
6. For comparison, look for an application-local Load Image event in **Procmon** if DebugView captures nothing. Sandboxed Spotify subprocesses may not send visible debug output.
7. After testing, exit Spotify and remove this **test** DLL. If Spotify fails to launch, immediately remove the probe DLL.

This probe deliberately does not create log files, because prior versions had silent file-write failures and this is a minimal loader/init test. If debug output remains absent despite verified loading, use an external debugger/ETW instead of assuming initialization did not run.

**Note:** The DLL export-forwarding implementation preserves the v0.6 signatures but has not been verified in a Windows build or tested in Spotify. Treat this as experimental.
