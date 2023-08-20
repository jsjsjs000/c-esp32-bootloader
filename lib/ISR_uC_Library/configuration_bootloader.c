#include "../../include/main.h"
#ifdef ESP32
	#include <stdio.h>
	#include <stdbool.h>
	#include <string.h>
	#include "esp_log.h"
	#include "esp_flash.h"
	#include "../ISR_ESP32_Library/flash.h"
#endif
#ifdef STM32F0
	#include "stm32f0xx_hal.h"
	#include "stm32f0xx_hal_flash_ex.h"
	#include "stm32f0xx_init.h"
#endif
#ifdef STM32F4
	#include "stm32f4xx_hal.h"
	#include "stm32f4xx_hal_flash_ex.h"
	#include "stm32f4xx_init.h"
#endif
#if defined(STM32F0) || defined(STM32F4)
	#include "../ISR_STM32_Library/flash.h"
#endif
#include "configuration.h"
#include "configuration_bootloader.h"

struct FlashConfiguration flashConfigurationBootloader;
struct ConfigurationBootloader cfgBootloader __attribute__((aligned(4)));

uint32_t GetRandomAddress(void)
{
	while (true)
	{
		uint32_t address = Random32();
		if (address != UINT32_MAX && address > 0xffff)
			return address;
	}
}

void InitializeConfigurationBootloader(struct ConfigurationBootloader* cfg, bool resetHardware)
{
	cfg->itemStoredSignature = CONFIGURATION_BOOTLOADER_ITEM_STORED_SIGNATURE;
#ifndef ESP32
	cfg->bootState = CONFIGURATION_BOOT_STATE_PROGRAM_NOT_EXISTS;
#endif
#ifdef BOOTLOADER
	cfg->bootloaderVersionMajor = BOOTLOADER_VERSION_MAJOR;
	cfg->bootloaderVersionMinor = BOOTLOADER_VERSION_MINOR;
	cfg->bootloaderYear = BOOTLOADER_YEAR;
	cfg->bootloaderMonth = BOOTLOADER_MONTH;
	cfg->bootloaderDay = BOOTLOADER_DAY;
#else
	cfg->bootloaderVersionMajor = 0;
	cfg->bootloaderVersionMinor = 0;
	cfg->bootloaderYear = 1;
	cfg->bootloaderMonth = 1;
	cfg->bootloaderDay = 1;
#endif
	cfg->programVersionMajor = 0;
	cfg->programVersionMinor = 0;
	cfg->programYear = 1;
	cfg->programMonth = 1;
	cfg->programDay = 1;
	cfg->programedProgramYear = 1;
	cfg->programedProgramMonth = 1;
	cfg->programedProgramDay = 1;
	cfg->programedProgramHour = 0;
	cfg->programedProgramMinute = 0;

	if (resetHardware)
	{
		cfg->hardwareType1 = ISR_HARDWARE_TYPE1;
		cfg->hardwareType2 = ISR_HARDWARE_TYPE2;
		cfg->hardwareSegmentsCount = ISR_SEGMENTS_COUNT;
		cfg->hardwareVersion = ISR_HARDWARE_VERSION;
		cfg->deviceAddress = GetRandomAddress();
		cfg->flashEncryptionKey = Random32();
	}
}

void InitializeFlashConfigurationBootloader(struct FlashConfiguration* flashCfg,
		struct ConfigurationBootloader* cfg)
{
	InitializeFlashConfiguration(flashCfg);
	flashCfg->itemStoredSignature = CONFIGURATION_BOOTLOADER_ITEM_STORED_SIGNATURE;
	flashCfg->configurationFlashAddress = CONFIGURATION_BOOTLOADER_FLASH_ADDRESS;
	flashCfg->configurationFlashSize = CONFIGURATION_BOOTLOADER_FLASH_SIZE;
	flashCfg->currentFlashAddress = 0;
	flashCfg->configurationPointer = cfg;
	flashCfg->configurationSize = sizeof(struct ConfigurationBootloader);
}

StatusType EraseFlashBootloader(void)
{
	return EraseFlashRegion(CONFIGURATION_BOOTLOADER_FLASH_ADDRESS, CONFIGURATION_BOOTLOADER_FLASH_SIZE);
}

void PrintFlashBootloaderConfiguration(struct ConfigurationBootloader* cfg)
{
	const char *TAG = "cfgBootloader";
	LOGI(TAG, "cfgBootloader 0x%08x ", (uint32_t)&cfg);
	LOGI(TAG, "  itemStoredSignature 0x%02x ", cfg->itemStoredSignature);
#ifndef ESP32
	LOGI(TAG, "  bootState %d ", cfg->bootState);
#endif
	LOGI(TAG, "  bootloaderVersionMajor %d ", cfg->bootloaderVersionMajor);
	LOGI(TAG, "  bootloaderVersionMinor %d ", cfg->bootloaderVersionMinor);
	LOGI(TAG, "  bootloaderYear %d ", cfg->bootloaderYear);
	LOGI(TAG, "  bootloaderMonth %d ", cfg->bootloaderMonth);
	LOGI(TAG, "  bootloaderDay %d ", cfg->bootloaderDay);
	LOGI(TAG, "  programVersionMajor %d ", cfg->programVersionMajor);
	LOGI(TAG, "  programVersionMinor %d ", cfg->programVersionMinor);
	LOGI(TAG, "  programYear %d ", cfg->programYear);
	LOGI(TAG, "  programMonth %d ", cfg->programMonth);
	LOGI(TAG, "  programDay %d ", cfg->programDay);
	LOGI(TAG, "  programedProgramYear %d ", cfg->programedProgramYear);
	LOGI(TAG, "  programedProgramMonth %d ", cfg->programedProgramMonth);
	LOGI(TAG, "  programedProgramDay %d ", cfg->programedProgramDay);
	LOGI(TAG, "  programedProgramHour %d ", cfg->programedProgramHour);
	LOGI(TAG, "  programedProgramMinute %d ", cfg->programedProgramMinute);

	LOGI(TAG, "  hardwareType1 %d ", cfg->hardwareType1);
	LOGI(TAG, "  hardwareType2 %d ", cfg->hardwareType2);
	LOGI(TAG, "  hardwareSegmentsCount %d ", cfg->hardwareSegmentsCount);
	LOGI(TAG, "  hardwareVersion %d ", cfg->hardwareVersion);
	LOGI(TAG, "  deviceAddress 0x%08x ", cfg->deviceAddress);
	LOGI(TAG, "  flashEncryptionKey 0x%08x ", cfg->flashEncryptionKey);
}

/*	fill configuration in flash for test
	if (flashConfigurationBootloader.currentFlashAddress >= 0x003fd050 &&
			flashConfigurationBootloader.currentFlashAddress <  0x003fd070 && foundCfgBootloader == ESP_OK)
	{
		while (flashConfigurationBootloader.currentFlashAddress < CONFIGURATION_BOOTLOADER_FLASH_ADDRESS + 8112)
			WriteConfigurationToFlash(&flashConfigurationBootloader);
	}
*/
