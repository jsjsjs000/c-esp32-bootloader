#ifndef INC_ISR_BOOTLOADER_H_
#define INC_ISR_BOOTLOADER_H_

#include <stdio.h>

#define UINT32_3BYTE(A)   (((A) >> 24) & 0xff)
#define UINT32_2BYTE(A)   (((A) >> 16) & 0xff)
#define UINT32_1BYTE(A)   (((A) >> 8) & 0xff)
#define UINT32_0BYTE(A)   ((A) & 0xff)

#define UINT32_FROM_BYTES(BYTE3, BYTE2, BYTE1, BYTE0) ((BYTE3) << 24 | (BYTE2) << 16 | (BYTE1) << 8 | BYTE0)

#define CONFIGURATION_BOOT_STATE_ITEM_STORED_SIGNATURE	0xe9
#define CONFIGURATION_BOOT_STATE_FLASH_ADDRESS					0x3ff000
#define CONFIGURATION_BOOT_STATE_FLASH_SIZE							0x1000

#define CONFIGURATION_BOOT_STATE_PROGRAM_NOT_EXISTS			0
#define CONFIGURATION_BOOT_STATE_PROGRAM_EXISTS					1
#define CONFIGURATION_BOOT_STATE_PROGRAM_EXISTS_AND_RAN	2
#define CONFIGURATION_BOOT_STATE_PROGRAM_UNKNOWN				0xff

extern void PrintMemory(const char* TAG, uint32_t address, uint32_t size, const uint8_t* buffer);
extern uint8_t GetBootState();
extern bool SetBootState(uint8_t bootState);
extern int GetActivePartitionFromBootState();

#endif /* INC_ISR_BOOTLOADER_H_ */
