#ifndef INC_CONFIGURATION_VISUAL_COMPONENTS_H_
#define INC_CONFIGURATION_VISUAL_COMPONENTS_H_

#include "../../include/main.h"
#if !defined(BOOTLOADER) && defined(ISR_CU)

#define CONFIGURATION_VISUAL_COMPONENTS_ITEM_STORED_SIGNATURE		0xed

#define CONFIGURATION_VISUAL_COMPONENTS_FLASH_ADDRESS						0x3f8000
#define CONFIGURATION_VISUAL_COMPONENTS_FLASH_SIZE							0x2000

// struct ConfigurationVisualComponents
// {
// 	uint8_t itemStoredSignature; /// ItemStoredSignature - must be - item identiefier for search config
// //	uint8_t itemStoredNumber;    /// ItemStoredNumber - must be - for example address
// };

extern struct FlashConfiguration flashConfigurationVisualComponents;
extern uint8_t *cfgVisualComponents;
extern uint16_t cfgVisualComponentsItemsCount;

extern void InitializeFlashConfigurationVisualComponents(struct FlashConfiguration *flashCfg);
extern StatusType EraseFlashVisualComponents(void);
extern void PrintFlashVisualComponentsConfiguration(uint8_t *cfg);
// extern uint16_t CalculateVisualComponentsCrc32(uint16_t cfgSize);
#endif

#endif /* INC_CONFIGURATION_VISUAL_COMPONENTS_H_ */
