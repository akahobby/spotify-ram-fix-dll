# Spotify RAM Fix v0.4 — Experimental DLL proxy

**Unverified Windows build / experimental.** Source adapts the previously tested `version.dll` probe and v0.1 working-set trimmer. The DLL forwards the 16 Version APIs from the probe, so this is not a drop-in replacement for all possible exports of Windows `version.dll` on every build. Spotify updates may break proxy loading.

## Setup

1. Remove the old **probe** `version.dll` from `%APPDATA%\Spotify` (back it up if desired). Never replace any DLL in `C:\Windows\System32`.
2. Upload contents of this folder to your GitHub repository root (including `.github/workflows/build.yml`).
3. Run the GitHub Actions workflow **Build Spotify RAM Fix proxy DLL**, download the artifact and extract it.
4. Exit Spotify completely. Put the new `version.dll` and `settings.ini` beside `%APPDATA%\Spotify\Spotify.exe`, only if there is no legitimate existing `version.dll` there.
5. Start Spotify, play music, and inspect `%LOCALAPPDATA%\SpotifyRamFix-memory-log.csv` with Notepad or Excel. Watch `trim_succeeded` and `win32_error` per PID.
6. To uninstall: completely close Spotify, delete **only the `version.dll` and `settings.ini` placed by this test**, then reopen Spotify.

## What it does

- Uses a **session-local mutex** so ordinarily just one injected Spotify subprocess runs the worker.
- Every 30 seconds (configurable between 5 sec and 1 hour) enumerates `Spotify.exe` processes and calls `EmptyWorkingSet` on each accessible process.
- Logs the before/after resident working set and any Windows errors. Reloads settings each interval.
- `EnableTrim=0` for monitoring only; `LogCsv=0` to stop logging.
- No MMCSS hooks, thread scheduling adjustments, input hooks, external injector, or Spotify executable modification.

## Critical limitations

- Your probe loaded inside a **low-integrity storage utility subprocess**. Windows may **deny** `OpenProcess` access to medium-integrity Spotify processes (`win32_error=5`), which means the DLL will **not** match the standalone EXE's 7/7 trim success. Examine CSV results first.
- A named mutex avoids duplicate work for processes that can access the mutex, but processes in different security contexts could still get access denied and run no worker; the log reveals this.
- Trimming affects Windows' **resident working set**, not the amount of memory Spotify allocates. Pages may be faulted back, with possible UI/audio latency.
- This is only 16 common Version API exports; if Spotify requires other exports, launching could fail. Test and roll back before everyday use.
- This source has not been compiled or executed on Windows in this environment.
