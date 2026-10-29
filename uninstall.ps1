$ErrorActionPreference = 'Stop'
$target = Join-Path $env:APPDATA 'Spotify'
$state = Join-Path $target 'SpotifyRamFix-install-state.json'
if (Get-Process Spotify -ErrorAction SilentlyContinue) { throw 'Completely exit Spotify (including tray) before uninstalling.' }
if (!(Test-Path $state)) { throw 'No installer record found. To avoid deleting another application DLL, uninstall manually after verifying file ownership.' }
$data = Get-Content -LiteralPath $state -Raw | ConvertFrom-Json
$dll = Join-Path $target 'version.dll'
$config = Join-Path $target 'settings.ini'
if ((Test-Path $dll) -and (Get-FileHash $dll -Algorithm SHA256).Hash -ne $data.DllHash) { throw 'version.dll has changed since installation. Uninstall manually to protect the replacement DLL.' }
if (Test-Path $dll) { Remove-Item -LiteralPath $dll }
if ($data.DllBackup) { Move-Item -LiteralPath (Join-Path $target $data.DllBackup) -Destination $dll }
if ((Test-Path $config) -and (Get-FileHash $config -Algorithm SHA256).Hash -eq $data.ConfigHash) {
  Remove-Item -LiteralPath $config
  if ($data.ConfigBackup) { Move-Item -LiteralPath (Join-Path $target $data.ConfigBackup) -Destination $config }
} else { Write-Warning 'settings.ini was modified; kept existing configuration. Restore backup manually if desired.' }
Remove-Item -LiteralPath $state
Write-Host 'Spotify RAM Fix v1.0 removed. Start Spotify normally.' -ForegroundColor Green
