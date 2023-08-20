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
#include "configuration_program.h"
#include "crc32.h"

struct FlashConfiguration flashConfigurationProgram;
struct ConfigurationProgram cfgProgram __attribute__((aligned(4)));

void InitializeConfigurationProgram(volatile struct ConfigurationProgram* cfg)
{
	cfg->itemStoredSignature = CONFIGURATION_PROGRAM_ITEM_STORED_SIGNATURE;
}

void InitializeFlashConfigurationProgram(struct FlashConfiguration* flashCfg,
		struct ConfigurationProgram* cfg)
{
	InitializeFlashConfiguration(flashCfg);
	flashCfg->itemStoredSignature = CONFIGURATION_PROGRAM_ITEM_STORED_SIGNATURE;
	flashCfg->configurationFlashAddress = CONFIGURATION_PROGRAM_FLASH_ADDRESS;
	flashCfg->configurationFlashSize = CONFIGURATION_PROGRAM_FLASH_SIZE;
	flashCfg->currentFlashAddress = 0;
	flashCfg->configurationPointer = cfg;
	flashCfg->configurationSize = sizeof(struct ConfigurationProgram);
}

StatusType EraseFlashProgram(void)
{
	return EraseFlashRegion(CONFIGURATION_PROGRAM_FLASH_ADDRESS, CONFIGURATION_PROGRAM_FLASH_SIZE);
}

void PrintFlashProgramConfiguration(struct ConfigurationProgram* cfg)
{
	const char *TAG = "cfgProgram";
	LOGI(TAG, "cfgProgram 0x%08x ", (uint32_t)&cfg);
	LOGI(TAG, "  itemStoredSignature 0x%02x ", cfg->itemStoredSignature);
}

uint16_t CalculateProgramCrc32(void)
{
	uint8_t cfgSize = sizeof(struct ConfigurationProgram) - 1;
	return CalculateCrc32(0, (uint8_t*)&cfgProgram, 1, cfgSize);
}
