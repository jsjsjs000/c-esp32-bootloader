#ifndef INC_CONFIGURATION_BOOTLOADER_H_
#define INC_CONFIGURATION_BOOTLOADER_H_

#include "../../include/main.h"

#define CONFIGURATION_BOOTLOADER_ITEM_STORED_SIGNATURE	0xea

#ifdef ESP32
	#define CONFIGURATION_BOOTLOADER_FLASH_ADDRESS	0x3fe000
	#define CONFIGURATION_BOOTLOADER_FLASH_SIZE			0x1000
#endif
#ifdef STM32F0
	#define CONFIGURATION_BOOTLOADER_FLASH_ADDRESS	\
			(FLASH_BASE + FLASH_PAGES_COUNT * FLASH_PAGE_SIZE - CONFIGURATION_BOOTLOADER_FLASH_SIZE)
	#define CONFIGURATION_BOOTLOADER_FLASH_SIZE			(2 * FLASH_PAGE_SIZE)
#endif

#define CONFIGURATION_BOOT_STATE_PROGRAM_NOT_EXISTS     0
#define CONFIGURATION_BOOT_STATE_PROGRAM_EXISTS         1
#define CONFIGURATION_BOOT_STATE_PROGRAM_EXISTS_AND_RAN 2
#define CONFIGURATION_BOOT_STATE_PROGRAM_UNKNOWN        0xff

#define CONFIGURATION_HARDWARE_TYPE1_NONE				0
#define CONFIGURATION_HARDWARE_TYPE1_COMMON			1
#define CONFIGURATION_HARDWARE_TYPE1_DIN				2
#define CONFIGURATION_HARDWARE_TYPE1_BOX				3
#define CONFIGURATION_HARDWARE_TYPE1_RADIO_BOX	4

#define CONFIGURATION_HARDWARE_TYPE2_NONE					0
#define CONFIGURATION_HARDWARE_TYPE2_CU						1
#define CONFIGURATION_HARDWARE_TYPE2_CU_WR				2
#define CONFIGURATION_HARDWARE_TYPE2_EXPANDER			3
#define CONFIGURATION_HARDWARE_TYPE2_RADIO				4
#define CONFIGURATION_HARDWARE_TYPE2_AMPLIFIER		5
#define CONFIGURATION_HARDWARE_TYPE2_ACIN					41
#define CONFIGURATION_HARDWARE_TYPE2_ANIN					42
#define CONFIGURATION_HARDWARE_TYPE2_ANOUT				43
#define CONFIGURATION_HARDWARE_TYPE2_DIGIN				44
#define CONFIGURATION_HARDWARE_TYPE2_DIM					45
#define CONFIGURATION_HARDWARE_TYPE2_LED					46
#define CONFIGURATION_HARDWARE_TYPE2_MUL					47
#define CONFIGURATION_HARDWARE_TYPE2_REL					48
#define CONFIGURATION_HARDWARE_TYPE2_ROL					49
#define CONFIGURATION_HARDWARE_TYPE2_TEMP					50
#define CONFIGURATION_HARDWARE_TYPE2_TABLET				81
#define CONFIGURATION_HARDWARE_TYPE2_TOUCH_PANEL	82

struct ConfigurationBootloader
{
	uint8_t itemStoredSignature; /// ItemStoredSignature - must be - item identiefier for search config
	//uint8_t itemStoredNumber;    /// ItemStoredNumber - must be - for example address
#ifndef ESP32
	uint8_t bootState;
#endif
	uint8_t bootloaderVersionMajor;
	uint8_t bootloaderVersionMinor;
	uint8_t bootloaderYear;
	uint8_t bootloaderMonth;
	uint8_t bootloaderDay;
	uint8_t programVersionMajor;
	uint8_t programVersionMinor;
	uint8_t programYear;
	uint8_t programMonth;
	uint8_t programDay;
	uint8_t programedProgramYear;
	uint8_t programedProgramMonth;
	uint8_t programedProgramDay;
	uint8_t programedProgramHour;
	uint8_t programedProgramMinute;

	uint8_t hardwareType1;
	uint8_t hardwareType2;
	uint8_t hardwareSegmentsCount;
	uint8_t hardwareVersion;
	uint32_t deviceAddress;
	uint32_t flashEncryptionKey;
};

extern struct FlashConfiguration flashConfigurationBootloader;
extern struct ConfigurationBootloader cfgBootloader; /// __attribute__((aligned(4)));

extern uint32_t GetRandomAddress(void);
extern void InitializeConfigurationBootloader(struct ConfigurationBootloader* cfg, bool resetHardware);
extern void InitializeFlashConfigurationBootloader(struct FlashConfiguration* flashCfg,
		struct ConfigurationBootloader* cfg);
extern StatusType EraseFlashBootloader(void);
extern void PrintFlashBootloaderConfiguration(struct ConfigurationBootloader* cfg);

#endif /* INC_CONFIGURATION_BOOTLOADER_H_ */
