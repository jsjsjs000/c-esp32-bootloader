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
#include "history_state.h"

struct FlashHistoryState flashHistoryState;

void InitializeFlashHistoryState(struct FlashHistoryState *flashCfg)
{
	flashCfg->itemStoredSignature = FLASH_HISTORY_STATE_ITEM_STORED_SIGNATURE;
	flashCfg->configurationFlashAddress = FLASH_HISTORY_STATE_FLASH_ADDRESS;
	flashCfg->configurationFlashSize = FLASH_HISTORY_STATE_FLASH_SIZE;
	flashCfg->configurationSize = FLASH_HISTORY_STATE_ITEM_SIZE;
	flashCfg->headerExtraBytes = FLASH_HISTORY_STATE_HEADER_EXTRA_BYTES;
	flashCfg->footerExtraBytes = FLASH_HISTORY_STATE_FOOTER_EXTRA_BYTES;
	flashCfg->currentFlashAddress = 0;
	flashCfg->notSendCurrentItemAddress = 0;
	// flashCfg->areNotSendHistoryItems = false;
}

uint16_t GetHistoryStateItemSize(struct FlashHistoryState *flashCfg)
{
	return 1 + flashCfg->headerExtraBytes + flashCfg->configurationSize + flashCfg->footerExtraBytes;
}

StatusType WriteHistoryStateToFlash(struct FlashHistoryState *flashCfg, uint8_t item[])
{
	uint16_t currentPage = flashCfg->currentFlashAddress / FLASH_PAGE_SIZE;
	uint16_t nextPage = (flashCfg->currentFlashAddress + 2 * GetHistoryStateItemSize(flashCfg)) / FLASH_PAGE_SIZE;

		/// is end of flash
	if (flashCfg->currentFlashAddress + 2 * GetHistoryStateItemSize(flashCfg) >=
			flashCfg->configurationFlashAddress + flashCfg->configurationFlashSize)
	{
			/// erase first page
		if (EraseFlashRegion(flashCfg->configurationFlashAddress, FLASH_PAGE_SIZE) != STATUS_OK)
			return STATUS_ERROR;
	}
		/// is next flash page (4096)
	else if (nextPage != currentPage)
	{
			/// erase next page
		uint32_t pageAddress = nextPage  *FLASH_PAGE_SIZE;
		if (EraseFlashRegion(pageAddress, FLASH_PAGE_SIZE) != STATUS_OK)
			return STATUS_ERROR;
	}

		/// write signature
	uint32_t writeAddress = flashCfg->currentFlashAddress;
	if (WriteToFlash(writeAddress, 1, &flashCfg->itemStoredSignature) != STATUS_OK)
		return STATUS_ERROR;

		/// skip signature and extra header bytes
	writeAddress += 1 + FLASH_HISTORY_STATE_HEADER_EXTRA_BYTES;

		/// write item to flash
	if (WriteToFlash(writeAddress, flashCfg->configurationSize, item) != STATUS_OK)
		return STATUS_ERROR;

	 writeAddress += flashCfg->configurationSize;

		/// write footer bytes
	if (flashCfg->footerExtraBytes > 0)
	{
		uint8_t footer[] = { 0 };
		if (WriteToFlash(writeAddress, flashCfg->footerExtraBytes, footer) != STATUS_OK)
			return STATUS_ERROR;

		writeAddress += flashCfg->footerExtraBytes;
	}

		/// go back to first page
	if (writeAddress + GetHistoryStateItemSize(flashCfg) >=
			flashCfg->configurationFlashAddress + flashCfg->configurationFlashSize)
		writeAddress = flashCfg->configurationFlashAddress;

	flashCfg->currentFlashAddress = writeAddress;

	if (flashCfg->currentFlashAddress == flashCfg->notSendCurrentItemAddress)
	{
		flashCfg->notSendCurrentItemAddress += GetHistoryStateItemSize(flashCfg);
		if (flashCfg->notSendCurrentItemAddress >=
				flashCfg->configurationFlashAddress + flashCfg->configurationFlashSize)
			flashCfg->notSendCurrentItemAddress = flashCfg->configurationFlashAddress;
	}

	// flashCfg->areNotSendHistoryItems = true;
	return STATUS_OK;
}

static void SetNotSendCurrentItemAddress(struct FlashHistoryState *flashCfg, uint32_t address)
{
	flashCfg->notSendCurrentItemAddress = address;
	// flashCfg->areNotSendHistoryItems = true;
}

FlashFindStatus FindHistoryStateInFlash(struct FlashHistoryState *flashCfg)
{
	uint8_t readBuffer;
	uint32_t address = flashCfg->configurationFlashAddress;

	while (address < flashCfg->configurationFlashAddress + flashCfg->configurationFlashSize)
	{
		if (ReadFromFlash(address, 1, &readBuffer) != STATUS_OK)
			return FIND_ERROR;

		if (readBuffer == flashCfg->itemStoredSignature)
		{
				/// item found - go to next item
			address += GetHistoryStateItemSize(flashCfg);
		}
		else if (readBuffer == 0xff)
		{
				/// found empty item space
			flashCfg->currentFlashAddress = address;
			SetNotSendCurrentItemAddress(flashCfg, flashCfg->currentFlashAddress);
			return FIND_OK;
		}
		else
		{
				/// found unknown item signature - erase first flash sector
			if (EraseFlashRegion(flashCfg->configurationFlashAddress, FLASH_PAGE_SIZE) != STATUS_OK)
				return FIND_ERROR;

			flashCfg->currentFlashAddress = flashCfg->configurationFlashAddress;
			SetNotSendCurrentItemAddress(flashCfg, flashCfg->currentFlashAddress);
			return FIND_OK;
		}
	}

		/// no empty space - reset first flash sector
	if (EraseFlashRegion(flashCfg->configurationFlashAddress, FLASH_PAGE_SIZE) != STATUS_OK)
		return FIND_ERROR;

	flashCfg->currentFlashAddress = flashCfg->configurationFlashAddress;
	SetNotSendCurrentItemAddress(flashCfg, flashCfg->currentFlashAddress);
	return FIND_OK;
}

bool FindNotSendHistoryState(struct FlashHistoryState *flashCfg)
{
	uint8_t readBuffer[2];
	uint32_t address = flashCfg->notSendCurrentItemAddress;

	while (address != flashCfg->currentFlashAddress)
	{
		if (ReadFromFlash(address, 2, readBuffer) != STATUS_OK)
			return false;

			/// item found
		if (readBuffer[0] == flashCfg->itemStoredSignature)
		{
			if (readBuffer[1] != 0xff)
			{
					/// item sent - go to next item
				address += GetHistoryStateItemSize(flashCfg);
			}
			else
			{
					/// item not send
				flashCfg->notSendCurrentItemAddress = address;
				return true;
			}
		}
		else if (readBuffer[0] == 0xff)
		{
				/// found empty item space - go to next item
			address += GetHistoryStateItemSize(flashCfg);
		}
		else
		{
				/// found unknown item signature - go to next item
			address += GetHistoryStateItemSize(flashCfg);
		}

			/// address greater than end address - go to begin
		if (address >= flashCfg->configurationFlashAddress + flashCfg->configurationFlashSize)
		{
			address = flashCfg->configurationFlashAddress;
		}
	}

	return false;
}

StatusType SetSendHistoryState(struct FlashHistoryState *flashCfg)
{
	const uint8_t buffer = 0;
	if (WriteToFlash(flashCfg->notSendCurrentItemAddress + 1, 1, &buffer) != STATUS_OK)
	{
		return STATUS_ERROR;
	}

		/// incremental address
	flashCfg->notSendCurrentItemAddress += GetHistoryStateItemSize(flashCfg);
		/// address greater than end address - go to begin
	if (flashCfg->notSendCurrentItemAddress >=
			flashCfg->configurationFlashAddress + flashCfg->configurationFlashSize)
	{
		flashCfg->notSendCurrentItemAddress = flashCfg->configurationFlashAddress;
	}

	return STATUS_OK;
}
