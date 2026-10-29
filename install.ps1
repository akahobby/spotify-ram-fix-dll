# Run from the same directory as version.dll and settings.ini.
$ErrorActionPreference = 'Stop'
$target = Join-Path $env:APPDATA 'Spotify'
$dll = Join-Path $target 'version.dll'
$sourceDll = Join-Path $PSScriptRoot 'version.dll'
$sourceConfig = Join-Path $PSScriptRoot 'settings.ini'
$state = Join-Path $target 'SpotifyRamFix-install-state.json'
if (!(Test-Path $sourceDll) -or !(Test-Path $sourceConfig)) { throw 'Extract the compiled release ZIP first; version.dll and settings.ini must be beside this script.' }
if (!(Test-Path (Join-Path $target 'Spotify.exe'))) { throw "Website Spotify.exe not found at $target" }
if (Get-Process Spotify -ErrorAction SilentlyContinue) { throw 'Completely exit Spotify (including tray) before installing.' }
if (Test-Path $state) { throw 'Existing SpotifyRamFix installation record found. Uninstall the previous version before installing this one.' }
$backup = $null
if (Test-Path $dll) {
  # Never overwrite a DLL of unknown origin without a restorable backup.
  $backup = 'version.dll.pre-SpotifyRamFix-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.bak'
  Copy-Item -LiteralPath $dll -Destination (Join-Path $target $backup) -ErrorAction Stop
  Write-Host "Backed up existing version.dll to $backup"
}
$configPath = Join-Path $target 'settings.ini'
$configBackup = $null
if (Test-Path $configPath) {
  $configBackup = 'settings.ini.pre-SpotifyRamFix-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.bak'
  Copy-Item -LiteralPath $configPath -Destination (Join-Path $target $configBackup) -ErrorAction Stop
}
Copy-Item -LiteralPath $sourceDll -Destination $dll -Force
Copy-Item -LiteralPath $sourceConfig -Destination $configPath -Force
@{DllHash=(Get-FileHash $dll -Algorithm SHA256).Hash; ConfigHash=(Get-FileHash $configPath -Algorithm SHA256).Hash; DllBackup=$backup; ConfigBackup=$configBackup; Version='1.0'} | ConvertTo-Json | Set-Content -LiteralPath $state -Encoding UTF8
Write-Host 'Installed Spotify RAM Fix v1.0. Start Spotify normally.' -ForegroundColor Green
