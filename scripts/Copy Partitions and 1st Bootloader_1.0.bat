@echo off
set version=1.0
set outpath="D:\Desktop\ESP32\Programs bins"
copy /Y "..\.pio\build\ISR_ESP32_BOOTLOADER\partitions.bin" %outpath%\partitions_%version%.bin
copy /Y "..\.pio\build\ISR_ESP32_BOOTLOADER\bootloader.bin" %outpath%\bootloader_%version%.bin
