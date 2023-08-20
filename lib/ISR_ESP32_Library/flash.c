#include "../../include/main.h"
#ifdef ESP32
	#include <stdio.h>
	#include "esp_log.h"
	#include "esp_err.h"
	#include "esp_partition.h"
#endif

StatusType ReadFromFlash(uint32_t address, uint16_t length, uint8_t *buffer)
{
	if (esp_flash_read(esp_flash_default_chip, buffer, address, length) != ESP_OK)
		return STATUS_ERROR;

	return STATUS_OK;
}

StatusType WriteToFlash(uint32_t address, uint16_t length, const uint8_t *buffer)
{
	if (esp_flash_write(esp_flash_default_chip, buffer, address, length) != ESP_OK)
		return STATUS_ERROR;

	return STATUS_OK;
}

StatusType EraseFlashRegion(uint32_t address, uint16_t length)
{
// ESP_LOGI("FLASH", "ERASE FLASH %x, %x", address, length);
	if (esp_flash_erase_region(esp_flash_default_chip, address, length) != ESP_OK)
		return STATUS_ERROR;

	return STATUS_OK;
}
