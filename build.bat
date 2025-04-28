@echo off
setlocal enabledelayedexpansion

set source_files=
for /R "src" %%f in (*.c) do (
    set source_files=!source_files! %%f
)

set bin_name=app

set bin_path=bin
if not exist %bin_path% (
    mkdir %bin_path%
)

:: compile config
set compile_flags=-g -Wvarargs -Wall -Werror
set includes=-Isrc -I%VULKAN_SDK%/Include
set links=-luser32 -lvulkan-1 -L%VULKAN_SDK%/Lib

echo Building %bin_name%...

call clang %source_files% %compile_flags% -o %bin_path%/%bin_name%.exe %includes% %links%