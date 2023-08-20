#ifndef INC_CONFIGURATION_CU_CONTROL_H_
#define INC_CONFIGURATION_CU_CONTROL_H_

#include "../../include/main.h"
#if !defined(BOOTLOADER) && defined(ISR_CU)

#define CONFIGURATION_CU_CONTROL_ITEM_STORED_SIGNATURE	0xec

#define CONFIGURATION_CU_CONTROL_FLASH_ADDRESS					0x3fa000
#define CONFIGURATION_CU_CONTROL_FLASH_SIZE							0x2000

// struct ConfigurationCuControl
// {
// 	uint8_t itemStoredSignature; /// ItemStoredSignature - must be - item identiefier for search config
// //	uint8_t itemStoredNumber;    /// ItemStoredNumber - must be - for example address
// };

extern struct FlashConfiguration flashConfigurationCuControl;
extern uint8_t *cfgCuControl;
extern uint16_t cfgCuControlItems;
// extern uint16_t cfgCuControlSize;

// extern void InitializeConfigurationCuControl(volatile uint8_t *cfg, uint16_t cfgSize);
extern void InitializeFlashConfigurationCuControl(struct FlashConfiguration *flashCfg);
extern StatusType EraseFlashCuControl(void);
extern void PrintFlashCuControlConfiguration(uint8_t *cfg);
// extern uint16_t CalculateCuControlCrc32(uint16_t cfgSize);
#endif

#endif /* INC_CONFIGURATION_CU_CONTROL_H_ */
