@echo off
rem Double-click to update Midi4u. It runs update.ps1, because Windows won't run a .ps1 on double-click. Keep this file next to Midi4u.exe and update.ps1.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0update.ps1"
