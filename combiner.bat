@echo off
setlocal enabledelayedexpansion

:: Define paths
set "SCRIPT_PATH=.\scripts\combine_bins.py"
set "BUILD_DIR=.\.pio\build"

for /d /r "%BUILD_DIR%" %%d in (*) do (
    if exist "%%d\firmware.bin" if exist "%%d\bootloader.bin" if exist "%%d\partitions.bin" (
        echo Found all files in: %%d

        :: Correctly format the command
        python "%SCRIPT_PATH%" --bin_path "%%d\bootloader.bin" --bin_address "0x1000" --bin_path "%%d\partitions.bin" --bin_address "0x8000" --bin_path "%%d\firmware.bin" --bin_address "0x10000"
    )
)

echo Operation completed.
endlocal
