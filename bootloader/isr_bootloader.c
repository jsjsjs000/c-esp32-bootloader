#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "sdkconfig.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "bootloader_flash_priv.h"

#include "isr_bootloader.h"

const char *TAG_BOOTLOADER = "BOOTLOADER_BOOTSTATE";

const uint32_t Crc32Seed = 0x7e5291fb;
uint32_t Crc32Table[256];
bool CrcTableInitialized = false;

void PrintMemory(const char* TAG, uint32_t address, uint32_t size, const uint8_t* buffer)
{
	for (uint32_t i = 0; i < size; i += 8)
		ESP_LOGI(TAG_BOOTLOADER, "0x%08x: %02x %02x %02x %02x  %02x %02x %02x %02x ", address + i,
				(uint8_t)buffer[i + 0], (uint8_t)buffer[i + 1], (uint8_t)buffer[i + 2], (uint8_t)buffer[i + 3],
				(uint8_t)buffer[i + 4], (uint8_t)buffer[i + 5], (uint8_t)buffer[i + 6], (uint8_t)buffer[i + 7]);
}

void InitializeCrc32Table()
{
	if (CrcTableInitialized)
		return;

	uint32_t crc, d;
	for (uint32_t i = 0; i < 256; i++)
	{
		crc = 0;
		d = i;
		for (int j = 0; j < 8; j++)
		{
			if (((crc ^ d) & 0x0001) != 0)
				crc = (crc >> 1) ^ Crc32Seed;
			else
				crc >>= 1;
			d >>= 1;
		}
		Crc32Table[i] = crc;
	}
	CrcTableInitialized = true;
}

uint32_t CalculateCrc32(uint32_t crc, uint8_t* data, int16_t from, int16_t length)
{
	for (int32_t i = from; i < from + length; i++)
		crc = (crc >> 8) ^ Crc32Table[(crc ^ data[i]) & 0xff];
	return crc;
}

uint8_t GetBootState()
{
	InitializeCrc32Table();

	int bootState = CONFIGURATION_BOOT_STATE_PROGRAM_UNKNOWN;

	uint8_t sizeRead = 8;
	uint8_t readBuffer[sizeRead];
	if (bootloader_flash_read(CONFIGURATION_BOOT_STATE_FLASH_ADDRESS, &readBuffer, sizeRead, true) != ESP_OK)
		return bootState;
	
	if (readBuffer[0] != CONFIGURATION_BOOT_STATE_ITEM_STORED_SIGNATURE)
		return bootState;

	uint32_t crcCalculated = CalculateCrc32(0, readBuffer, 0, sizeRead - 4);
	uint32_t crc = UINT32_FROM_BYTES(readBuffer[sizeRead - 1], readBuffer[sizeRead - 2],
			readBuffer[sizeRead - 3], readBuffer[sizeRead - 4]);
// PrintMemory(TAG_BOOTLOADER, CONFIGURATION_BOOT_STATE_FLASH_ADDRESS, sizeRead, &readBuffer[0]);
// ESP_LOGI(TAG_BOOTLOADER, "crc calc: %x, crc: %x ", crcCalculated, crc);

	if (crcCalculated != crc)
		return bootState;
	
	bootState = readBuffer[1];
	ESP_LOGI(TAG_BOOTLOADER, "boot state: %d ", bootState);
	
	return bootState;
}

bool SetBootState(uint8_t bootState)
{
	InitializeCrc32Table();

	if (bootloader_flash_erase_range(CONFIGURATION_BOOT_STATE_FLASH_ADDRESS,
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
// PrintMemory(TAG_BOOTLOADER, CONFIGURATION_BOOT_STATE_FLASH_ADDRESS, sizeWrite, &writeBuffer[0]);
// ESP_LOGI(TAG_BOOTLOADER, "write crc: %x ", crcCalculated);

	if (bootloader_flash_write(CONFIGURATION_BOOT_STATE_FLASH_ADDRESS, &writeBuffer, sizeWrite, false) != ESP_OK)
		return false;

	return true;
}

int GetActivePartitionFromBootState()
{
	int bootIndex = -1;	/// default program or bootloader
	uint8_t bootState = GetBootState();
	if (bootState == CONFIGURATION_BOOT_STATE_PROGRAM_EXISTS ||
			bootState == CONFIGURATION_BOOT_STATE_PROGRAM_EXISTS_AND_RAN)
		bootIndex = 3;		/// program
// bootIndex = -1;	/// force default program or bootloader
// bootIndex = 3;		/// force program

	if (bootState == CONFIGURATION_BOOT_STATE_PROGRAM_EXISTS)
		SetBootState(CONFIGURATION_BOOT_STATE_PROGRAM_NOT_EXISTS);

	ESP_LOGI(TAG_BOOTLOADER, "boot index: %d ", bootIndex);
	return bootIndex;
}
