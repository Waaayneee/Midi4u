@echo off
rem Double-click to install Midi4u. It runs update.ps1, because Windows won't run a .ps1 on double-click.
if not exist "%~dp0update.ps1" (
    echo Extract the zip first: right-click it, choose Extract All, then run this file from the extracted folder.
    pause
    exit /b
)
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0update.ps1" -Install
