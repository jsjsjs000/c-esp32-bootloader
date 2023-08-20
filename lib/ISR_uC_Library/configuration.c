#include "../../include/main.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#ifdef ESP32
	#include "esp_log.h"
	#include "esp_err.h"
	#include "esp_flash.h"
	#include "../ISR_ESP32_Library/flash.h"
#endif
#ifdef STM32F0
	#include "stm32f0xx_hal.h"
	#include "stm32f0xx_hal_flash_ex.h"
#endif
#ifdef STM32F4
	#include "stm32f4xx_hal.h"
	#include "stm32f4xx_hal_flash_ex.h"
#endif
#if defined(STM32F0) || defined(STM32F4)
	#include "../ISR_STM32_Library/flash.h"
#endif
#include "common.h"
#include "crc32.h"
#include "configuration.h"

uint32_t GetFlashAddressForPage(struct FlashConfiguration* flashCfg, uint8_t page)
{
	return flashCfg->configurationFlashAddress + page * (flashCfg->configurationFlashSize / FLASH_PAGE_SIZE);
}

void InitializeFlashConfiguration(struct FlashConfiguration* flashCfg)
{
	flashCfg->itemStoredSignature = 0xff;
	flashCfg->currentFlashAddress = 0;
	flashCfg->configurationFlashAddress = 0xffffffffU;
	flashCfg->configurationFlashSize = 0;
	flashCfg->configurationSize = 0;
	flashCfg->configurationPointer = NULL;
	flashCfg->configurationChanged = false;
}

StatusType EraseConfigurationFlash(struct FlashConfiguration* flashCfg, uint8_t page)
{
#ifdef ESP32
	return EraseFlashRegion(GetFlashAddressForPage(flashCfg, page), FLASH_PAGE_SIZE);
#endif
#if defined(STM32F0) || defined(STM32F4)
	return EraseFlashRegion(GetFlashAddressForPage(flashCfg, page), FLASH_PAGE_SIZE);
#endif
}

StatusType WriteConfigurationToFlash(struct FlashConfiguration* flashCfg)
{
		/// get current flash address
	bool eraseRestFlash = false;
	if (flashCfg->currentFlashAddress == 0)
		flashCfg->currentFlashAddress = flashCfg->configurationFlashAddress;
	else
		flashCfg->currentFlashAddress += ALIGN_UP_TO_4(flashCfg->configurationSize + 4);
	if (flashCfg->currentFlashAddress == 0)
		return STATUS_ERROR;

		/// next write configuration to flash overflow size
	if (flashCfg->currentFlashAddress + ALIGN_UP_TO_4(flashCfg->configurationSize + 4) >=
			flashCfg->configurationFlashAddress + flashCfg->configurationFlashSize)
	{
			/// erase first flash page
		if (EraseFlashRegion(flashCfg->configurationFlashAddress, FLASH_PAGE_SIZE) != STATUS_OK)
			return STATUS_ERROR;

		flashCfg->currentFlashAddress = flashCfg->configurationFlashAddress;
		eraseRestFlash = true;
	}

		/// write configuration to flash
	if (WriteToFlash(flashCfg->currentFlashAddress, ALIGN_UP_TO_4(flashCfg->configurationSize),
			(void*)flashCfg->configurationPointer) != STATUS_OK)
		return STATUS_ERROR;

		/// write CRC32 of configuration to flash
	uint32_t crc = CalculateCrc32(0, (void*)flashCfg->configurationPointer, 0,
			ALIGN_UP_TO_4(flashCfg->configurationSize));
	if (WriteToFlash(flashCfg->currentFlashAddress + ALIGN_UP_TO_4(flashCfg->configurationSize),
			sizeof(uint32_t), (void*)&crc) != STATUS_OK)
		return STATUS_ERROR;

		/// if erase first flash page, then erase rest flash pages
	if (eraseRestFlash)
	{
		uint32_t from = flashCfg->configurationFlashAddress + FLASH_PAGE_SIZE;
		uint32_t to = flashCfg->configurationFlashAddress + flashCfg->configurationFlashSize;
		if (to > from)
			if (EraseFlashRegion(from, to - from) != STATUS_OK)
				return STATUS_ERROR;
	}

	return STATUS_OK;
}

uint32_t GetConfigurationMaxItemsCount(struct FlashConfiguration* flashCfg)
{
	return flashCfg->configurationFlashSize / ALIGN_UP_TO_4(flashCfg->configurationSize + 4);
}

static FlashFindStatus FindConfigurationInFlashAtAddress(struct FlashConfiguration* flashCfg, uint32_t *adr)
{
	if (*adr == 0)
		return FIND_NONE;

	uint8_t readSignature;
	if (ReadFromFlash(*adr, sizeof(readSignature), &readSignature) != STATUS_OK)
		return FIND_ERROR;

	if (readSignature != flashCfg->itemStoredSignature)
		return FIND_NONE;

	uint32_t configurationItemSize = ALIGN_UP_TO_4(flashCfg->configurationSize);
	uint8_t readData[configurationItemSize];
	if (ReadFromFlash(*adr, configurationItemSize, readData) != STATUS_OK)
		return FIND_ERROR;

	uint32_t crcCalculated = CalculateCrc32(0, readData, 0, sizeof(readData));

	uint32_t crcRead;
	if (ReadFromFlash(*adr + configurationItemSize, sizeof(uint32_t), (uint8_t*)&crcRead) != STATUS_OK)
		return FIND_ERROR;

	if (crcCalculated != crcRead)
		return FIND_NONE;

	uint8_t *pCfg;
	pCfg = (uint8_t*)(flashCfg->configurationPointer);
	memcpy(pCfg, readData, configurationItemSize);

	flashCfg->currentFlashAddress = *adr;

	return FIND_OK;
}

FlashFindStatus FindConfigurationInFlash(struct FlashConfiguration* flashCfg)
{
	uint32_t configurationItemSize = ALIGN_UP_TO_4(flashCfg->configurationSize);
	uint32_t maxItemsCount = GetConfigurationMaxItemsCount(flashCfg);
	uint32_t startAdr = flashCfg->configurationFlashAddress;
	uint32_t adr = startAdr + (configurationItemSize + 4) * (maxItemsCount - 1);
	while (adr >= startAdr)
	{
		FlashFindStatus findStatus = FindConfigurationInFlashAtAddress(flashCfg, &adr);
		if (findStatus == FIND_OK)
			return FIND_OK;
		else if (findStatus == FIND_ERROR)
			return FIND_ERROR;

		adr -= configurationItemSize + 4;
	}

	return FIND_NONE;
}
