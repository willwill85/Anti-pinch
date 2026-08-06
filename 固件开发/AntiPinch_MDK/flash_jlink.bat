@echo off
setlocal
cd /d "%~dp0"
"%ProgramFiles%\SEGGER\JLink_V944\JLink.exe" -NoGui 1 -CommanderScript flash_jlink.jlink
endlocal
