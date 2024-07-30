@echo off
set MSYS2_PATH=C:\msys64
set QEMU_PATH=%MSYS2_PATH%\mingw64\bin
set FIRMWARE_FILE=%~dp0.pio\build\esp32dev\firmware.bin

set PATH=%QEMU_PATH%;%PATH%
bash -c "qemu-system-xtensa -nographic -machine esp32 -m 4M -drive file=%FIRMWARE_FILE%,if=mtd,format=raw -gdb tcp::1234 -S"