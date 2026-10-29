# Spotify RAM Fix v0.9 — Experimental cross-process trim

A Windows x64 `version.dll` proxy that builds on the verified v0.8 implementation. It forwards Windows Version API calls to the System32 DLL. The DLL starts a per-instance background worker, trims its own working set, then **attempts** to trim other running Spotify.exe processes installed at the **same executable path**. It neither modifies their code nor changes MMCSS or thread priorities.

## Build on GitHub

Upload `version_proxy.cpp`, `version.def`, `CMakeLists.txt`, `settings.ini`, and `.github/workflows/build.yml` to the repository root. Run the workflow **Build Spotify RAM Fix DLL v0.9**. Download its compiled Windows artifact. This source has not been Windows compiled or run in this environment.

## Install

1. Fully exit Spotify. Shut down any standalone RAM optimizer while testing.
2. Back up any existing custom `version.dll` in `%APPDATA%\Spotify` (never alter System32).
3. Copy the compiled `build/Release/version.dll` and `settings.ini` into `%APPDATA%\Spotify`.
4. Run Spotify and capture **Win32** output in Sysinternals DebugView. Look for `SpotifyRamFix v0.9` output and `target=<PID>` results.
5. Test playback, navigation, CPU usage and page faults. To revert, exit Spotify and remove the custom DLL, restoring the prior version if desired.

## Settings

`EnableTrim=1`: enable trimming. `TrimIntervalMs=30000`: interval (5s–1h clamped). `EnableCrossProcessTrim=1`: attempt sibling process trims. Set `EnableCrossProcessTrim=0` for v0.8-style self-only behavior.

## Limitations and safety

- **Not guaranteed to trim all seven processes.** Low-integrity Spotify subprocesses can be denied access to the main process or other subprocesses. DebugView reports `open_denied`, `path_open_failed`, or `trim_failed` with Windows errors. Do not bypass these security boundaries.
- Multiple DLL instances may target the same process each cycle. This is an intentionally simple, experimental coverage test, **not a production-efficient scheduler**. If paging or sluggishness increases, turn off cross-process mode or revert to v0.8.
- Working-set trimming reduces resident memory reported by Task Manager, **not private committed allocation**. Memory may be faulted back in, and excessive trimming can hurt performance.
- No MMCSS hooks, thread-priority changes, raw-input hooks, process patching, elevated privileges or automatic injection.
- The default app-local DLL proxy can break with Spotify updates; keep a rollback copy.
