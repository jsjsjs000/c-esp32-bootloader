#include "../../include/main.h"
#if !defined(BOOTLOADER) && defined(ISR_CU)
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "esp_log.h"
#include "esp_flash.h"
#include "../ISR_ESP32_Library/flash.h"
#include "common.h"
#include "configuration.h"
#include "configuration_visual_components.h"
#include "crc32.h"
#include "device_item2.h"

struct FlashConfiguration flashConfigurationVisualComponents;
uint8_t *cfgVisualComponents;
uint16_t cfgVisualComponentsItemsCount = 0;

void InitializeFlashConfigurationVisualComponents(struct FlashConfiguration *flashCfg)
{
	InitializeFlashConfiguration(flashCfg);
	flashCfg->itemStoredSignature = CONFIGURATION_VISUAL_COMPONENTS_ITEM_STORED_SIGNATURE;

	uint16_t cfgSize;
	VisualComponent_InitializeConfigurationVisualComponents(flashCfg, &cfgVisualComponentsItemsCount, &cfgSize);

	flashCfg->configurationFlashAddress = CONFIGURATION_VISUAL_COMPONENTS_FLASH_ADDRESS;
	flashCfg->configurationFlashSize = CONFIGURATION_VISUAL_COMPONENTS_FLASH_SIZE;
	flashCfg->currentFlashAddress = 0;
	flashCfg->configurationPointer = cfgVisualComponents;
	flashCfg->configurationSize = cfgSize;

	VisualComponent_WriteVisualComponentsToConfiguration(flashCfg);
}

StatusType EraseFlashVisualComponents(void)
{
	return EraseFlashRegion(CONFIGURATION_VISUAL_COMPONENTS_FLASH_ADDRESS, CONFIGURATION_VISUAL_COMPONENTS_FLASH_SIZE);
}

void PrintFlashVisualComponentsConfiguration(uint8_t *cfg)
{
	const char *TAG = "cfgVisualComponents";
	LOGI(TAG, "cfgVisualComponents 0x%08x ", (uint32_t)&cfg);
}

// uint16_t CalculateVisualComponentsCrc32(uint16_t cfgSize)
// {
// 	return CalculateCrc32(0, (uint8_t*)&cfgVisualComponents, 1, cfgSize - 1);
// }
#endif
