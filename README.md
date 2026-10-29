# Spotify RAM Fix v0.5 — diagnostic build

This is an **untested** diagnostic update to v0.4. It preserves the original cross-process trimming and mutex design so we can pinpoint why it isn't working. The DLL is a `version.dll` proxy; **do not put it in System32**.

## Build
Upload all files to the root of your GitHub repository, preserving `.github/workflows/build.yml`. Run **Actions > Build Spotify RAM Fix proxy DLL > Run workflow**, then download the `SpotifyRamFix-v0.5-x64` artifact.

## Install
1. Exit Spotify completely, including its tray icon.
2. Remove the old test `version.dll` from `%APPDATA%\Spotify`. Do not replace an unrelated pre-existing DLL.
3. Put the new `version.dll` and `settings.ini` next to `Spotify.exe` in `%APPDATA%\Spotify`.
4. Launch Spotify and play music for at least 45 seconds.
5. Check `%LOCALAPPDATA%\Low\SpotifyRamFix-diagnostic.log` and `%LOCALAPPDATA%\Low\SpotifyRamFix-memory-log.csv`. If absent, also check `%LOCALAPPDATA%` for those filenames.

## Interpreting results
- `worker_started`: the DLL spawned its background worker.
- `mutex_acquired`: this process owns the single optimizer loop.
- `mutex_already_owned`: another Spotify process owns it; normal for other processes.
- `trim_cycle_begin` and `trim_cycle_logged`: optimization cycle ran and attempted CSV output.
- In CSV, `win32_error=5` suggests access denied while trimming other Spotify processes.

If **neither log exists**, the worker may be unable to write even to LocalLow or may not have started. Check Procmon for the log filename and its Result, or use DebugView for `SpotifyRamFix: CreateThread failed`.

Remove only our `version.dll` and `settings.ini` if Spotify fails to open. Working-set trimming can reduce Task Manager's memory figure without reducing total committed memory and may increase page faults.
