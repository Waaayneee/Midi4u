# Installs Midi4u from this folder (-Install), or updates it to the newest GitHub release.
# Keep this file next to Midi4u.exe and run it with "Install Midi4u (click me).bat" or
# "Update Midi4u (click me).bat".
param([switch]$Install)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue' # the progress bar makes downloads very slow in Windows PowerShell

$repo = 'Waaayneee/Midi4u'
$appDir = $PSScriptRoot
$vst3Dir = Join-Path $env:CommonProgramW6432 'VST3' # 64-bit "C:\Program Files\Common Files"

function Finish($message) {
    Write-Host ''
    Write-Host $message
    Read-Host 'Press Enter to close'
    exit
}

# Copying the plug-in into Program Files needs admin rights, so ask once and restart as admin.
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
if (-not ([Security.Principal.WindowsPrincipal]$identity).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    $argList = "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`""
    if ($Install) { $argList += ' -Install' }
    try {
        Start-Process powershell -Verb RunAs -ArgumentList $argList
        exit
    } catch {
        Finish 'Midi4u needs admin rights to put the plug-in where Ableton finds it. Run it again and click Yes.'
    }
}

try {
    if (-not (Test-Path (Join-Path $appDir 'Midi4u.exe'))) {
        Finish "Put this file in the same folder as Midi4u.exe, then run it again. It is now in $appDir"
    }
    # "Start with Windows" remembers where Midi4u.exe is, so it must live somewhere permanent.
    if ($Install -and $appDir -like (Join-Path $env:USERPROFILE 'Downloads*')) {
        Finish 'Move this folder out of Downloads first (into Documents is good), then run the installer again.'
    }

    if (-not $Install) {
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        Write-Host 'Checking GitHub for the newest Midi4u...'
        $release = Invoke-RestMethod "https://api.github.com/repos/$repo/releases/latest"
        $version = $release.tag_name

        $versionFile = Join-Path $appDir 'version.txt'
        if ((Test-Path $versionFile) -and (Get-Content $versionFile -Raw).Trim() -eq $version) {
            Finish "Midi4u is already up to date ($version)."
        }

        $asset = $release.assets | Where-Object { $_.name -eq 'Midi4u-windows.zip' }
        if (-not $asset) { throw "Release $version has no Midi4u-windows.zip." }
    }

    # Ableton keeps the plug-in file locked while it is open.
    if (Get-Process 'Ableton Live*' -ErrorAction SilentlyContinue) {
        Finish 'Close Ableton Live first, then run this again.'
    }

    if (-not $Install) {
        $temp = Join-Path $env:TEMP 'Midi4u-update'
        Remove-Item $temp -Recurse -Force -ErrorAction SilentlyContinue
        New-Item -ItemType Directory $temp | Out-Null
        Write-Host "Downloading Midi4u $version..."
        Invoke-WebRequest $asset.browser_download_url -OutFile "$temp\update.zip" -UseBasicParsing
        Expand-Archive "$temp\update.zip" "$temp\files"
    }

    $running = Get-Process 'Midi4u' -ErrorAction SilentlyContinue
    if ($running) {
        Write-Host 'Closing Midi4u...'
        $running | Stop-Process -Force
        $running | Wait-Process -Timeout 10 -ErrorAction SilentlyContinue
    }

    Write-Host 'Installing...'
    $folderPlugin = Join-Path $appDir 'Midi4u Tap.vst3'
    if (-not $Install) {
        # Refresh this whole folder, so it always holds the newest version. Overwriting this
        # script is safe: PowerShell has already read it.
        if (Test-Path $folderPlugin) { Remove-Item $folderPlugin -Recurse -Force }
        Copy-Item "$temp\files\*" $appDir -Recurse -Force
    }
    if (-not (Test-Path $folderPlugin)) { throw "Midi4u Tap.vst3 is missing from $appDir. Download Midi4u again." }
    New-Item -ItemType Directory -Force $vst3Dir | Out-Null
    $oldPlugin = Join-Path $vst3Dir 'Midi4u Tap.vst3'
    if (Test-Path $oldPlugin) { Remove-Item $oldPlugin -Recurse -Force } # must be gone, or the copy nests inside it
    Copy-Item $folderPlugin $vst3Dir -Recurse -Force

    if (-not $Install) {
        Set-Content $versionFile $version
        Remove-Item $temp -Recurse -Force -ErrorAction SilentlyContinue
    }

    # Start through Explorer so Midi4u runs as you, not as admin.
    if ($Install -or $running) { Start-Process explorer.exe "`"$(Join-Path $appDir 'Midi4u.exe')`"" }
    if ($Install) {
        Finish 'Midi4u is installed and opening now. Next, let Ableton find the plug-in (README, Step 4).'
    }
    Finish "Midi4u is updated to $version."
} catch {
    Finish "Failed: $($_.Exception.Message)"
}
