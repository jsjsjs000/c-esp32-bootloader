#include "../../include/main.h"
#ifdef ESP32
	#include <stdio.h>
	#include "esp_err.h"
#endif

extern StatusType ReadFromFlash(uint32_t address, uint16_t length, uint8_t *buffer);
extern StatusType WriteToFlash(uint32_t address, uint16_t length, const uint8_t *buffer);
extern StatusType EraseFlashRegion(uint32_t address, uint16_t length);
