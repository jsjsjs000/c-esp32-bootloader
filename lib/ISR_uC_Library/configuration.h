#ifndef __CONFIGURATION_H
#define __CONFIGURATION_H

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

typedef enum
{
  FIND_OK     = 0x00U,
  FIND_ERROR  = 0x01U,
  FIND_NONE   = 0x02U
} FlashFindStatus;

#ifdef ESP32
	#define FLASH_PAGE_SIZE		4096
#endif

/*
	STM32F030C8T6: FLASH 64KB, RAM 8KB, page size 1KB
	FLASH_BASE       0x80000000UL // in stm32f0xx_hal_flash_ex.h
	FLASH_PAGE_SIZE  0x400U


	STM32F072RB: FLASH 128KB, RAM 16KB, page size 2KB
	FLASH_BASE       0x80000000UL // in stm32f0xx_hal_flash_ex.h
	FLASH_PAGE_SIZE  0x800U

	#define ADDR_FLASH_PAGE_0     ((uint32_t)0x08000000) // Base @ of Page 0, 2 Kbytes
	#define ADDR_FLASH_PAGE_1     ((uint32_t)0x08000800) // Base @ of Page 1, 2 Kbytes
	#define ADDR_FLASH_PAGE_2     ((uint32_t)0x08001000) // Base @ of Page 2, 2 Kbytes
	#define ADDR_FLASH_PAGE_3     ((uint32_t)0x08001800) // Base @ of Page 3, 2 Kbytes
	...
	#define ADDR_FLASH_PAGE_60    ((uint32_t)0x0801E000) // Base @ of Page 60, 2 Kbytes
	#define ADDR_FLASH_PAGE_61    ((uint32_t)0x0801E800) // Base @ of Page 61, 2 Kbytes
	#define ADDR_FLASH_PAGE_62    ((uint32_t)0x0801F000) // Base @ of Page 62, 2 Kbytes
	#define ADDR_FLASH_PAGE_63    ((uint32_t)0x0801F800) // Base @ of Page 63, 2 Kbytes


	STM32F401RCT6: FLASH 256KB, RAM 64KB
	FLASH_BASE       0x80000000UL // in stm32f0xx_hal_flash_ex.h
	FLASH_PAGE_SIZE  0x800U       // fixed size

	Sector 0 0x08000000 - 0x08003FFF 16 Kbytes  - Bootloader
	Sector 1 0x08004000 - 0x08007FFF 16 Kbytes
	Sector 2 0x08008000 - 0x0800BFFF 16 Kbytes  - Program configuration
	Sector 3 0x0800C000 - 0x0800FFFF 16 Kbytes  - Bootloader configuration
	Sector 4 0x08010000 - 0x0801FFFF 64 Kbytes  - Bootloader
	Sector 5 0x08020000 - 0x0803FFFF 128 Kbytes - Program code
*/

#ifdef STM32F0
	#ifdef STM32F030x8
		#define FLASH_PAGES_COUNT		64
	#endif
	#ifdef STM32F091xC
		#define FLASH_PAGES_COUNT		128
	#endif

	#define FLASH_ERASE_PAGE_TIME	40 /// ms
	#define FLASH_ERASE_PAGE_TIME_TO_COUNT	80 /// ms
#endif
#ifdef STM32F4
	#define FLASH_SECTOR_ADDR_0	0x08000000U
	#define FLASH_SECTOR_ADDR_1	0x08004000U
	#define FLASH_SECTOR_ADDR_2	0x08008000U
	#define FLASH_SECTOR_ADDR_3	0x0800C000U
	#define FLASH_SECTOR_ADDR_4	0x08010000U
	#define FLASH_SECTOR_ADDR_5	0x08020000U
	#define FLASH_SECTOR_SIZE_0	(16 * 1024)		/// 0x4000
	#define FLASH_SECTOR_SIZE_1	(16 * 1024)		/// 0x4000
	#define FLASH_SECTOR_SIZE_2	(16 * 1024)		/// 0x4000
	#define FLASH_SECTOR_SIZE_3	(16 * 1024)		/// 0x4000
	#define FLASH_SECTOR_SIZE_4	(64 * 1024)		/// 0x10000
	#define FLASH_SECTOR_SIZE_5	(128 * 1024)	/// 0x20000

	#define FLASH_ERASE_PAGE_TIME	40 /// ms
	#define FLASH_ERASE_PAGE_TIME_TO_COUNT	600 /// ms
#endif

struct FlashConfiguration
{
	uint8_t itemStoredSignature;				/// ItemStoredSignature - must be - item identiefier for search config
	uint32_t currentFlashAddress;
	uint32_t configurationFlashAddress;
	uint32_t configurationFlashSize;
	uint16_t configurationSize;
	void *configurationPointer;
	bool configurationChanged;
};

extern void InitializeFlashConfiguration(struct FlashConfiguration* flashCfg);
extern StatusType WriteConfigurationToFlash(struct FlashConfiguration* flashCfg);
extern FlashFindStatus FindConfigurationInFlash(struct FlashConfiguration* flashCfg);

#endif /* __CONFIGURATION_H */
