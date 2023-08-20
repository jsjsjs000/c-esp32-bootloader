@echo off
set filename="ISR-DIN-CU_1.0.bin"
set outpath="D:\Desktop\ESP32\Programs bins"
copy /Y "..\.pio\build\ISR_ESP32_BOOTLOADER\firmware.bin" %outpath%\%filename%
