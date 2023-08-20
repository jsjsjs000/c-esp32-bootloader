#ifndef INC_CONFIGURATION_PROGRAM_H_
#define INC_CONFIGURATION_PROGRAM_H_

#include "../../include/main.h"
#include "configuration_bootloader.h"

#define CONFIGURATION_PROGRAM_ITEM_STORED_SIGNATURE 0xeb

#ifdef ESP32
	#define CONFIGURATION_PROGRAM_FLASH_ADDRESS					0x3fc000
	#define CONFIGURATION_PROGRAM_FLASH_SIZE						0x2000
#endif
#ifdef STM32F0
	#define CONFIGURATION_PROGRAM_FLASH_ADDRESS	\
			(FLASH_BASE + FLASH_PAGES_COUNT * FLASH_PAGE_SIZE - CONFIGURATION_BOOTLOADER_FLASH_SIZE - CONFIGURATION_PROGRAM_FLASH_SIZE)
	#define CONFIGURATION_PROGRAM_FLASH_SIZE		(2 * FLASH_PAGE_SIZE)
#endif

struct ConfigurationProgram
{
	uint8_t itemStoredSignature; /// ItemStoredSignature - must be - item identiefier for search config
//	uint8_t itemStoredNumber;    /// ItemStoredNumber - must be - for example address
};

extern struct FlashConfiguration flashConfigurationProgram;
extern struct ConfigurationProgram cfgProgram; /// __attribute__((aligned(4)))

extern void InitializeConfigurationProgram(volatile struct ConfigurationProgram* cfg);
extern void InitializeFlashConfigurationProgram(struct FlashConfiguration* flashCfg,
		struct ConfigurationProgram* cfg);
extern StatusType EraseFlashProgram(void);
extern void PrintFlashProgramConfiguration(struct ConfigurationProgram* cfg);
extern uint16_t CalculateProgramCrc32(void);

#endif /* INC_CONFIGURATION_PROGRAM_H_ */
