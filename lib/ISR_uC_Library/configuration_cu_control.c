#include "../../include/main.h"
#if !defined(BOOTLOADER) && defined(ISR_CU)
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "esp_log.h"
#include "esp_flash.h"
#include "../ISR_ESP32_Library/flash.h"
#include "configuration.h"
#include "configuration_cu_control.h"
#include "crc32.h"
#include "device_item.h"

struct FlashConfiguration flashConfigurationCuControl;
uint8_t *cfgCuControl;
uint16_t cfgCuControlItemsCount = 0;

void InitializeFlashConfigurationCuControl(struct FlashConfiguration *flashCfg)
{
	InitializeFlashConfiguration(flashCfg);
	flashCfg->itemStoredSignature = CONFIGURATION_CU_CONTROL_ITEM_STORED_SIGNATURE;

	uint16_t cfgSize;
	DeviceItem_InitializeConfigurationControls(flashCfg, &cfgCuControlItemsCount, &cfgSize);

	flashCfg->configurationFlashAddress = CONFIGURATION_CU_CONTROL_FLASH_ADDRESS;
	flashCfg->configurationFlashSize = CONFIGURATION_CU_CONTROL_FLASH_SIZE;
	flashCfg->currentFlashAddress = 0;
	flashCfg->configurationPointer = cfgCuControl;
	flashCfg->configurationSize = cfgSize;

	DeviceItem_WriteControlToConfiguration(flashCfg);
}

StatusType EraseFlashCuControl(void)
{
	return EraseFlashRegion(CONFIGURATION_CU_CONTROL_FLASH_ADDRESS, CONFIGURATION_CU_CONTROL_FLASH_SIZE);
}

void PrintFlashCuControlConfiguration(uint8_t *cfg)
{
	const char *TAG = "cfgCuControl";
	LOGI(TAG, "cfgCuControl 0x%08x ", (uint32_t)&cfg);
}

// uint16_t CalculateCuControlCrc32(uint16_t cfgSize)
// {
// 	return CalculateCrc32(0, (uint8_t*)&cfgCuControl, 1, cfgSize - 1);
// }
#endif
