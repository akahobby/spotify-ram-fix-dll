# Spotify RAM Fix v0.6 (experimental)

A `version.dll` proxy for the **website-installed Windows x64 Spotify client**. This is a test build, not a verified fix.

## What changed

- Removed the cross-process mutex and attempts to open other Spotify processes.
- Each Spotify subprocess that loads this DLL trims **only its own** working set.
- Uses `%USERPROFILE%\AppData\LocalLow\SpotifyRamFix\` for diagnostics and CSV output (the prior version mistakenly used `Local\Low`).
- Logs worker startup, memory samples, and trim failures. If file logging fails, emits `OutputDebugString` messages.
- Keeps version API forwarding, with no MMCSS, thread priority, or raw input hooks.

## Build

Upload the files to your repository root and keep `.github/workflows/build.yml` in the correct location. Use **Actions → Build Spotify RAM Fix DLL v0.6 → Run workflow**. Download `SpotifyRamFix-v0.6-x64`.

## Test

1. Fully exit Spotify (including tray and background processes).
2. In `%APPDATA%\Spotify`, **back up any existing third-party `version.dll`** and remove the old test DLL. Never touch `C:\Windows\System32\version.dll`.
3. Copy the new `version.dll` and `settings.ini` beside `Spotify.exe`.
4. Launch Spotify and play music for at least a minute. Stop using any standalone Spotify RAM trimmer while testing.
5. Read `%USERPROFILE%\AppData\LocalLow\SpotifyRamFix\diagnostic.log` and `memory-log.csv`.
6. Check Task Manager. Some Spotify processes may load the system version DLL instead; this implementation **will not trim those**.
7. If startup or playback breaks, close Spotify and remove the custom `version.dll`.

`memory-log.csv` columns: timestamp, pid, before_bytes, after_bytes, trim_succeeded, win32_error. Trimming a working set lowers reported resident memory but does **not** fix leaks or reduce underlying committed memory. Frequent trimming can increase page faults and playback glitches.

## Limitations

- Experimental and untested on Windows. GitHub build needs to compile successfully.
- This DLL starts a worker via `CreateThread` during `DllMain`, but does not wait or perform file I/O there; the worker performs work after loader initialization. Fully deferred initialization would require a guaranteed post-load callback from the host, which we have not established.
- Exact version API forwarding compatibility is not guaranteed across all versions of Spotify.
