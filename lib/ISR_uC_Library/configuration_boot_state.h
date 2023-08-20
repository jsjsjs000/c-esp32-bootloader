#ifndef INC_CONFIGURATION_BOOT_STATE_H_
#define INC_CONFIGURATION_BOOT_STATE_H_

#include "../../include/main.h"

#ifdef ESP32
	#define CONFIGURATION_BOOT_STATE_ITEM_STORED_SIGNATURE	0xe9
	#define CONFIGURATION_BOOT_STATE_FLASH_ADDRESS					0x3ff000
	#define CONFIGURATION_BOOT_STATE_FLASH_SIZE							0x1000

	extern uint8_t GetBootState();
	extern bool SetBootState(uint8_t bootState);
	extern int GetActivePartitionFromBootState();
#endif

#endif /* INC_CONFIGURATION_BOOT_STATE_H_ */
