# Spotify Version DLL Probe — test only

This project is an experimental, x64 proxy for the Windows `version.dll` API. It **does not optimize RAM**. It only logs that Spotify loaded it, and forwards 16 commonly-used version API exports to the original Windows DLL. It has **not been run on Windows or validated against every export of your system's version.dll**. A successful build does not guarantee Spotify compatibility.

## Build

Upload all files to GitHub, including `.github/workflows/build.yml` as a real path (create it through GitHub's *Create new file* page if hidden folders cause trouble). Open **Actions > Build Windows DLL probe > Run workflow**. Download the `SpotifyVersionProbe-x64` artifact, and extract `version.dll`.

## Before testing

1. Close Spotify completely (including system tray). In Task Manager, verify `Spotify.exe` is gone.
2. In Task Manager open Spotify process Properties, or open its file location, to locate the folder that contains `Spotify.exe` (website-installed version).
3. **Do not proceed if the folder already contains `version.dll`; never overwrite any existing file.** Never place this in Windows/System32 or SystemWOW64.
4. Back up any Spotify application files before experimentation. Spotify can update or repair its installation and replace this test DLL.
5. Copy this probe `version.dll` into the folder containing `Spotify.exe`. No admin rights should be needed when the folder belongs to the current user.
6. Start Spotify. Inspect `%LOCALAPPDATA%\SpotifyVersionProbe.log` (paste path into Win+R or Explorer). One or more entries with PIDs means it loaded. You can also verify in Procmon by filtering `Process Name is Spotify.exe` and `Operation is Load Image`, looking at the precise path.
7. If no log appears, Spotify may have loaded the system DLL directly or a security policy may have prevented app-local loading. Do not claim success unless you observe an app-local `Load Image` or a newly recorded PID.
8. **After the test**, close Spotify completely and delete only the probe `version.dll` you copied there. Restart Spotify to confirm normal operation.

## Failure recovery

If Spotify crashes or fails to open, close it and delete ONLY the newly placed probe `version.dll` from Spotify's folder. Do not delete the system `version.dll`. This proxy intentionally avoids the MMCSS, thread priority and raw-input hooks from the Discord project. It does not trim memory.

Note: This experimental proxy does not promise perfect export/ordinal compatibility with the Windows system DLL. Use for a short, reversible test only.
