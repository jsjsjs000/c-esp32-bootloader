#include "../../include/main.h"

#ifdef ESP32
	#include <stdio.h>
	#include <stdbool.h>
	#include <string.h>
	#include "esp_log.h"
	#include "esp_flash.h"
	#include "../ISR_ESP32_Library/flash.h"
	#include "configuration_boot_state.h"
	#include "configuration_bootloader.h"
	#include "common.h"
	#include "crc32.h"

	const char *TAG_BOOTLOADER = "__BOOT_STATE";

	uint8_t GetBootState()
	{
		int bootState = CONFIGURATION_BOOT_STATE_PROGRAM_UNKNOWN;

		uint8_t sizeRead = 8;
		uint8_t readBuffer[sizeRead];
		if (esp_flash_read(esp_flash_default_chip, &readBuffer, CONFIGURATION_BOOT_STATE_FLASH_ADDRESS, sizeRead) != ESP_OK)
			return bootState;

		if (readBuffer[0] != CONFIGURATION_BOOT_STATE_ITEM_STORED_SIGNATURE)
			return bootState;

		uint32_t crcCalculated = CalculateCrc32(0, readBuffer, 0, sizeRead - 4);
		uint32_t crc = UINT32_FROM_BYTES(readBuffer[sizeRead - 1], readBuffer[sizeRead - 2],
				readBuffer[sizeRead - 3], readBuffer[sizeRead - 4]);
// printMemory(TAG_BOOTLOADER, CONFIGURATION_BOOT_STATE_FLASH_ADDRESS, sizeRead, &readBuffer[0]);
// ESP_LOGI(TAG_BOOTLOADER, "crc calc: %x ", crcCalculated);
// ESP_LOGI(TAG_BOOTLOADER, "crc: %x ", crc);

		if (crcCalculated != crc)
			return bootState;

		bootState = readBuffer[1];
// ESP_LOGI(TAG_BOOTLOADER, "bootState: %d ", bootState);

		return bootState;
	}

	bool SetBootState(uint8_t bootState)
	{
		if (esp_flash_erase_region(esp_flash_default_chip, CONFIGURATION_BOOT_STATE_FLASH_ADDRESS,
				CONFIGURATION_BOOT_STATE_FLASH_SIZE) != ESP_OK)
			return false;

		uint8_t sizeWrite = 8;
		uint8_t writeBuffer[sizeWrite];
		writeBuffer[0] = CONFIGURATION_BOOT_STATE_ITEM_STORED_SIGNATURE;
		writeBuffer[1] = bootState;
		writeBuffer[2] = 0;
		writeBuffer[3] = 0;

		uint32_t crcCalculated = CalculateCrc32(0, writeBuffer, 0, sizeWrite - 4);
		writeBuffer[sizeWrite - 4] = UINT32_0BYTE(crcCalculated);
		writeBuffer[sizeWrite - 3] = UINT32_1BYTE(crcCalculated);
		writeBuffer[sizeWrite - 2] = UINT32_2BYTE(crcCalculated);
		writeBuffer[sizeWrite - 1] = UINT32_3BYTE(crcCalculated);
// printMemory(TAG_BOOTLOADER, CONFIGURATION_BOOT_STATE_FLASH_ADDRESS, sizeWrite, &writeBuffer[0]);
// ESP_LOGI(TAG_BOOTLOADER, "write crc: %x ", crcCalculated);

		if (esp_flash_write(esp_flash_default_chip, &writeBuffer, CONFIGURATION_BOOT_STATE_FLASH_ADDRESS, sizeWrite) != ESP_OK)
			return false;

		return true;
	}
#endif
