#ifndef __HISTORY_STATE_H
#define __HISTORY_STATE_H

#include "../../include/main.h"
#ifdef ESP32
	#include "esp_err.h"
#endif
#ifdef STM32F0
	#include "stm32f0xx_hal.h"
	#include "stm32F0xx_ll_utils.h"
#endif
#ifdef STM32F4
	#include "stm32f4xx_hal.h"
	#include "stm32F4xx_ll_utils.h"
#endif
#include "configuration.h"

#define FLASH_HISTORY_STATE_ITEM_STORED_SIGNATURE		0xcf

#define FLASH_HISTORY_STATE_FLASH_ADDRESS						0x380000
#define FLASH_HISTORY_STATE_FLASH_SIZE							0x78000		/// 120 * 4KB
#define FLASH_HISTORY_STATE_ITEM_SIZE								369
// #define FLASH_HISTORY_STATE_ITEM_SIZE								384       /// round up to 24*16=384
#define FLASH_HISTORY_STATE_HEADER_EXTRA_BYTES			1					/// is history state send do server
#define FLASH_HISTORY_STATE_FOOTER_EXTRA_BYTES			0					/// bytes count at end

struct FlashHistoryState
{
	uint8_t itemStoredSignature;				/// ItemStoredSignature - must be - item identiefier for search config
	uint32_t configurationFlashAddress;
	uint32_t configurationFlashSize;
	uint16_t configurationSize;					/// Item size
	uint8_t headerExtraBytes;
	uint8_t footerExtraBytes;
	uint32_t currentFlashAddress;				/// next empty address
	uint32_t notSendCurrentItemAddress;				/// current history sending address
	// bool areNotSendHistoryItems;							/// if don't send 
};

extern struct FlashHistoryState flashHistoryState;

extern void InitializeFlashHistoryState(struct FlashHistoryState *flashCfg);
extern uint16_t GetHistoryStateItemSize(struct FlashHistoryState *flashCfg);
extern StatusType WriteHistoryStateToFlash(struct FlashHistoryState *flashCfg, uint8_t item[]);
extern FlashFindStatus FindHistoryStateInFlash(struct FlashHistoryState *flashCfg);
extern bool FindNotSendHistoryState(struct FlashHistoryState *flashCfg);
extern StatusType SetSendHistoryState(struct FlashHistoryState *flashCfg);

#endif /* __HISTORY_STATE_H */
