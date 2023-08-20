@echo off
set filename="ISR-BOX-TEMP-2_1.0.bin"
set outpath="D:\Desktop\ESP32\Programs bins"
copy /Y "..\.pio\build\ISR_ESP32_BOOTLOADER\firmware.bin" %outpath%\Bootloader_%filename%
