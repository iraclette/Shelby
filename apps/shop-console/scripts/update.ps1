# Runs detached from ShopConsole.exe (see UpdateChecker::downloadAndInstall)
# after it has already quit — Windows locks a running .exe, so the swap has
# to happen from outside the process being replaced.
#
# Ships inside the release zip itself (next to the exe), so every update
# also refreshes this script for the next one.

param(
    [Parameter(Mandatory = $true)][string]$ZipPath,
    [Parameter(Mandatory = $true)][string]$InstallDir,
    [Parameter(Mandatory = $true)][string]$ExeName
)

$ErrorActionPreference = "Stop"
$exePath = Join-Path $InstallDir $ExeName
$backupDir = "${InstallDir}_update_backup"
$processName = [System.IO.Path]::GetFileNameWithoutExtension($ExeName)

# The app calls QProcess::startDetached() on this script and quits right
# after — give it a few seconds to actually exit rather than racing it.
$deadline = (Get-Date).AddSeconds(15)
while ((Get-Date) -lt $deadline) {
    if (-not (Get-Process -Name $processName -ErrorAction SilentlyContinue)) { break }
    Start-Sleep -Milliseconds 300
}

try {
    if (Test-Path $backupDir) { Remove-Item $backupDir -Recurse -Force }
    Copy-Item $InstallDir $backupDir -Recurse -Force

    Expand-Archive -Path $ZipPath -DestinationPath $InstallDir -Force

    Remove-Item $backupDir -Recurse -Force
    Remove-Item $ZipPath -Force -ErrorAction SilentlyContinue
} catch {
    # Something went wrong mid-update — restore what was there before
    # rather than leaving a half-overwritten install that won't launch.
    if (Test-Path $backupDir) {
        Get-ChildItem $InstallDir -Force | Remove-Item -Recurse -Force
        Copy-Item "$backupDir\*" $InstallDir -Recurse -Force
        Remove-Item $backupDir -Recurse -Force
    }
}

Start-Process -FilePath $exePath
