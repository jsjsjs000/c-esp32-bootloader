#include <stdio.h>
#include <stdbool.h>
#include "esp_event.h"
#include "esp_log.h"
#include "tcp_server.h"
#ifdef ISR_CU
	#include "../lib/ISR_uC_Library/device_item2.h"
#endif

// #define BOOTLOADER		/// change partitions in file 'partitions_custom.csv' too

#define WATCHDOG

#include "../lib/ISR_uC_Library/isr_din_cu.h"
// #include "../lib/ISR_uC_Library/isr_box_temp2.h"

// #define CONFIGURATION_TEST
#define CONFIGURATION_ADAM_KUKUC
// #define CONFIGURATION_ADAM_KUKUC_TEST

#define WATCHDOG_RESET_TIME	1000

#define ESP32

#define StatusType esp_err_t
#define STATUS_OK ESP_OK
#define STATUS_ERROR ESP_FAIL

#define Random32() esp_random()					// uint32_t

#define LOGI(tag, format, ...)              \
do {                                        \
	ESP_LOGI(tag, format, ##__VA_ARGS__);     \
}                                           \
while (false)

#define LOGE(tag, format, ...)              \
do {                                        \
	ESP_LOGE(tag, format, ##__VA_ARGS__);     \
}                                           \
while (false)

#define ERROR_HANDLER()                                          \
{                                                                \
	ESP_LOGE(TAG, "%s:%d %s", __FILE__, __LINE__, __FUNCTION__);   \
	while (true) ;                                                 \
}

#define TICK_RATE_MS  portTICK_RATE_MS
#define GET_MS()      pdTICKS_TO_MS(xTaskGetTickCount())
#define DELAY_MS(ms)  vTaskDelay(pdMS_TO_TICKS(ms))

#define BOOTLOADER_VERSION_MAJOR	1
#define BOOTLOADER_VERSION_MINOR	0
#define BOOTLOADER_YEAR						22
#define BOOTLOADER_MONTH					10
#define BOOTLOADER_DAY						10

#define FLASH_PROGRAM_ADDRESS						0xc0000
#define FLASH_PROGRAM_PAGE_SIZE					4096
#define FLASH_ERASE_PAGE_TIME_TO_COUNT	30 /// ms

#ifdef ISR_TEMP
	#define TEMP1_GPIO_Port 13
	#define TEMP2_GPIO_Port 23
#endif

#ifdef ISR_DIN
	#define GPIO_LED 33
#endif
#ifdef ISR_BOX
	#define GPIO_LED 12
#endif

extern uint32_t uptime;               /// s
extern uint64_t miliseconds;          /// ms
extern bool isInitialized;
extern uint32_t lastWatchdogReset;
extern uint64_t ledFastBlink;         /// ms
extern uint16_t synchronizationDifference; /// ms

#ifdef ISR_TEMP
	extern int16_t DS18B20Temperatures[ISR_SEGMENTS_COUNT];
#endif

#ifdef ISR_CU
	extern struct Communication communicationTcp;
#endif
extern struct CommunicationCommandStruct communicationCommand1;
extern struct CommunicationCommandStruct communicationCommand2;

#if !defined(BOOTLOADER) && defined(ISR_CU)
	extern uint32_t writeCuControlTimestamp;
	extern uint32_t writeVisualControlsTimestamp;
#endif

extern void Error_Handler(void); // $$
extern void JumpToProgram_(void);
extern void ResetDevice(void);
extern void InitializeLed(void);
extern void LedTestLoop(void);

#ifdef ISR_TEMP
	extern void DS18B20_MeasureCallback(uint8_t number, int16_t temperature);
#endif

#ifdef ISR_CU
	extern void Lan8720a_EthEventHandler(void *arg, esp_event_base_t event_base,
			int32_t event_id, void *event_data);
	extern void Lan8720a_GotIpEventHandler(void *arg, esp_event_base_t event_base,
			int32_t event_id, void *event_data);

	extern uint16_t TcpServer_AnswerForRequest(int32_t sock, uint8_t* rxBuffer, uint16_t rxBufferLength,
			uint8_t* txBuffer, uint16_t txBufferLength);

	extern void Command_AnswerGetRelaysStatus(uint32_t address, bool answerOk, uint32_t uptime, uint16_t vin, bool *relays, uint8_t count);
	extern void Command_AnswerSetRelaysStatus(uint32_t address, bool answerOk);
	extern void Command_AnswerGetTemperatures(uint32_t address, bool answerOk, uint32_t uptime, uint16_t vin, uint16_t *temperatures, uint8_t count);
#endif


/*
	public enum HardwareType1Enum
	{
		None = 0,
		Common = 1,
		DIN = 2,
		BOX = 3,
		RadioBOX = 4,
	};

	public enum HardwareType2Enum
	{
		None = 0,
		CU = 1,
		CU_WR = 2,
		Expander = 3,
		Radio = 4,
		Amplifier = 5,
		Acin = 41,
		Anin = 42,
		Anout = 43,
		Digin = 44,
		Dim = 45,
		Led = 46,
		Mul = 47,
		Rel = 48,
		Rol = 49,
		Temp = 50,
		Tablet = 81,
		TouchPanel = 82,
	};
*/
