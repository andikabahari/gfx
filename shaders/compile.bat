@echo off
setlocal

:: Get the directory of the current script
set SCRIPT_DIR=%~dp0

call "glslc.exe" "%SCRIPT_DIR%shader.vert" -o "%SCRIPT_DIR%vert.spv"
call "glslc.exe" "%SCRIPT_DIR%shader.frag" -o "%SCRIPT_DIR%frag.spv"

endlocal