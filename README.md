# Spotify RAM Fix v0.9.3 — coordinator update

Experimental C++ `version.dll` proxy for the website-installed Spotify desktop app. This update is based on the user-tested v0.9.2 code.

## Changes

- Elects **one cross-process coordinator per Windows logon session** via a named mutex, held for the worker's lifetime.
- Other proxy-loaded Spotify processes continue trimming **their own** working sets on the configured interval, without each enumerating all sibling PIDs.
- If the coordinator process terminates, another instance can take over at its next interval.
- Existing configuration, Version API forwarding, and no-MMCSS/no-thread-hook behavior are retained.
- Default detailed debugging remains off. Enable `EnableDebugLogging=1` when diagnosing `coordinator_elected`, `trim_success`, and access-denied conditions.

## Important limitations

- The coordinator is selected from the Spotify subprocesses that actually load the application-local proxy DLL. It may have insufficient rights to trim other Spotify processes. **This is not an all-seven-process guarantee.**
- Self-trimming and coordinator trimming can occasionally overlap for a target process; this update prevents *multiple full scans*, not every duplicate trim across process boundaries.
- `EmptyWorkingSet` reduces resident working-set pages, not private committed allocations. Low Task Manager memory is not proof of a repaired memory leak.
- Native DLL proxy loading is experimental and may break when Spotify changes. Back up your known-working v0.9.2 binary and never overwrite `C:\Windows\System32\version.dll`.

## Build

Commit the project files to the root of the GitHub repository and run **Build Spotify RAM Fix DLL v0.9.3** in GitHub Actions. The uploaded artifact includes `build/Release/version.dll` and `settings.ini`.

## Install / rollback

1. Exit Spotify completely.
2. Back up `%APPDATA%\Spotify\version.dll` and `settings.ini` if present.
3. Copy the newly compiled `version.dll` and `settings.ini` beside the website-install Spotify.exe at `%APPDATA%\Spotify`.
4. Launch Spotify; for diagnosis set `EnableDebugLogging=1` and watch DebugView with Capture Win32 enabled.
5. If it crashes, fails to launch, or behaves poorly, exit Spotify, remove this custom DLL, or restore v0.9.2.

## Runtime defaults

30-second interval, 5-second first-trim delay, threshold disabled, and cross-process attempts enabled. MMCSS and thread-priority hooks are not present.
