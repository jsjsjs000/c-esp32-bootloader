#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_partition.h"
#include "esp_task_wdt.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "main.h"
#include "../lib/ISR_uC_Library/common.h"
#include "../lib/ISR_uC_Library/configuration.h"
#include "../lib/ISR_uC_Library/configuration_bootloader.h"
#include "../lib/ISR_uC_Library/configuration_program.h"
#include "../lib/ISR_uC_Library/configuration_boot_state.h"
#include "../lib/ISR_ESP32_Library/adc.h"
#ifdef ISR_CU
	#include "../lib/ISR_uC_Library/device_item2.h"
	#include "../lib/ISR_uC_Library/device_item.h"
	#include "../lib/ISR_ESP32_Library/lan8720a.h"
	#include "../lib/ISR_ESP32_Library/rtc_ds3231.h"
	#include "../lib/ISR_ESP32_Library/rtc.h"
#endif
#if !defined(BOOTLOADER) && defined(ISR_CU)
	#include "configuration_cu_control.h"
	#include "configuration_visual_components.h"
#endif
#ifdef ISR_TEMP
	#include "../lib/ISR_uC_Library/DS18B20.h"
#endif
#include "packets.h"
#include "timer.h"
#include "uart1.h"
#include "uart2.h"
#ifdef BOOTLOADER
	#include "communication_bootloader.h"
#endif
#include "communication.h"
#ifdef ISR_CU
	#include "tcp_server.h"
	#include "tcp_client.h"
	#include "direct_mode.h"
#endif

static const uint8_t LogLevel = 0; /// 0 - 2

#if defined(BOOTLOADER) && defined(ISR_CU)
	static const char *TAG = "ISR-DIN-CU-BOOTLOADER";
#elif !defined(BOOTLOADER) && defined(ISR_CU)
	static const char *TAG = "ISR-DIN-CU";
#elif defined(BOOTLOADER) && defined(ISR_TEMP)
	static const char *TAG = "ISR-BOX-TEMP-BOOTLOADER";
#elif !defined(BOOTLOADER) && defined(ISR_TEMP)
	static const char *TAG = "ISR-BOX-TEMP";
#endif

static volatile char ProgramIndicator[] =
{
	0x57, 0x9a, 0x93, 0xed,
	'S', 'm', 'a', 'r', 't', 'H', 'o', 'm', 'e', ' ',
#ifdef BOOTLOADER
	'B', 'o', 'o', 't', 'l', 'o', 'a', 'd', 'e', 'r', ' ',
#else
	'P', 'r', 'o', 'g', 'r', 'a', 'm', ' ',
#endif
	ISR_HARDWARE_TYPE, ' ',
	ISR_HARDWARE_NAME, ' ',
	ISR_HARDWARE_TYPE1, ISR_HARDWARE_TYPE2, ISR_SEGMENTS_COUNT,
#ifdef BOOTLOADER
	BOOTLOADER_VERSION_MAJOR, BOOTLOADER_VERSION_MINOR,
	BOOTLOADER_YEAR, BOOTLOADER_MONTH, BOOTLOADER_DAY
#else
	PROGRAM_VERSION_MAJOR, PROGRAM_VERSION_MINOR,
	PROGRAM_YEAR, PROGRAM_MONTH, PROGRAM_DAY
#endif
};

uint32_t uptime = 0;      /// s
uint64_t miliseconds = 0; /// ms
bool isInitialized = false;
//uint32_t configurationCrc32 = 0;
uint32_t lastWatchdogReset = 0;
uint64_t ledFastBlink = -1LL;  /// ms
uint16_t synchronizationDifference = 0; /// ms

#ifdef ISR_TEMP
	int16_t DS18B20Temperatures[ISR_SEGMENTS_COUNT];
#endif

#ifdef ISR_CU
	struct Communication communicationTcp;
#endif

#if !defined(BOOTLOADER) && defined(ISR_CU)
	uint32_t writeCuControlTimestamp = 0;
	uint32_t writeVisualControlsTimestamp = 0;
#endif

void JumpToProgram_(void)
{
	// DELAY_MS(30);
	// JumpToProgram();
}

void ResetDevice(void)
{
	// DELAY_MS(30);
	esp_restart();
}

void InitializeLed(void)
{
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = 1ULL << GPIO_LED;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpio_config(&io_conf);
	gpio_set_level(GPIO_LED, 1);
}

#ifdef ISR_TEMP
	void InitializeTemperatures(void)
	{
		gpio_config_t io_conf;
		io_conf.intr_type = GPIO_INTR_DISABLE;
		io_conf.mode = GPIO_MODE_INPUT_OUTPUT_OD;
		io_conf.pin_bit_mask = 1ULL << TEMP1_GPIO_Port;
		io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
		io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
		gpio_config(&io_conf);
		gpio_set_level(TEMP1_GPIO_Port, 1);

		io_conf.pin_bit_mask = 1ULL << TEMP2_GPIO_Port;
		gpio_config(&io_conf);
		gpio_set_level(TEMP2_GPIO_Port, 1);
	}
#endif

esp_err_t InitilizeConfiguration(void)
{
	InitializeFlashConfigurationBootloader(&flashConfigurationBootloader, &cfgBootloader);
	InitializeFlashConfigurationProgram(&flashConfigurationProgram, &cfgProgram);
#if !defined(BOOTLOADER) && defined(ISR_CU)
	InitializeFlashConfigurationCuControl(&flashConfigurationCuControl);
	InitializeFlashConfigurationVisualComponents(&flashConfigurationVisualComponents);
#endif

	if (FindConfigurationInFlash(&flashConfigurationBootloader) != FIND_OK)
	{
			/// No configuration found - initialize it
#ifdef BOOTLOADER
		InitializeConfigurationBootloader(&cfgBootloader, true);
#else
		InitializeConfigurationBootloader(&cfgBootloader, false);
#endif
		if (WriteConfigurationToFlash(&flashConfigurationBootloader) != STATUS_OK)
			ERROR_HANDLER();
	}

	if (LogLevel >= 2)
		PrintFlashBootloaderConfiguration(&cfgBootloader);

	// if (cfgBootloader.programVersionMajor != PROGRAM_VERSION_MAJOR ||
	// 		cfgBootloader.programVersionMinor != PROGRAM_VERSION_MINOR ||
	// 		cfgBootloader.programYear != PROGRAM_YEAR ||
	// 		cfgBootloader.programMonth != PROGRAM_MONTH ||
	// 		cfgBootloader.programDay != PROGRAM_DAY)
	// {
	// 	cfgBootloader.programVersionMajor = PROGRAM_VERSION_MAJOR;
	// 	cfgBootloader.programVersionMinor = PROGRAM_VERSION_MINOR;
	// 	cfgBootloader.programYear = PROGRAM_YEAR;
	// 	cfgBootloader.programMonth = PROGRAM_MONTH;
	// 	cfgBootloader.programDay = PROGRAM_DAY;

	// 	if (WriteConfigurationToFlash(&flashConfigurationBootloader) != STATUS_OK)
	// 		ERROR_HANDLER();
	// }

	if (FindConfigurationInFlash(&flashConfigurationProgram) != FIND_OK)
	{
			/// No configuration found - initialize it
		InitializeConfigurationProgram(&cfgProgram);
		if (WriteConfigurationToFlash(&flashConfigurationProgram) != STATUS_OK)
			ERROR_HANDLER();
	}

	if (LogLevel >= 2)
		PrintFlashProgramConfiguration(&cfgProgram);

#if !defined(BOOTLOADER) && defined(ISR_CU)
	if (FindConfigurationInFlash(&flashConfigurationCuControl) != FIND_OK)
	{
			/// No configuration found - initialize it
		if (WriteConfigurationToFlash(&flashConfigurationCuControl) != STATUS_OK)
			ERROR_HANDLER();
	}

	if (FindConfigurationInFlash(&flashConfigurationVisualComponents) != FIND_OK)
	{
			/// No configuration found - initialize it
		EraseFlashVisualComponents();
		if (WriteConfigurationToFlash(&flashConfigurationVisualComponents) != STATUS_OK)
			ERROR_HANDLER();
	}

	if (LogLevel >= 2)
	{
		PrintFlashCuControlConfiguration(cfgCuControl);
		PrintFlashVisualComponentsConfiguration(cfgVisualComponents);
		PrintFlashMemory(TAG, CONFIGURATION_CU_CONTROL_FLASH_ADDRESS, 128);
		PrintFlashMemory(TAG, CONFIGURATION_VISUAL_COMPONENTS_FLASH_ADDRESS, 128);
	}
#endif

// configurationCrc32 = CalculateProgramCrc32();

// if (foundCfgBootloader==STATUS_OK) WriteConfigurationToFlash(&flashConfigurationBootloader);

	if (LogLevel >= 2)
	{
		PrintFlashMemory(TAG, CONFIGURATION_BOOTLOADER_FLASH_ADDRESS, 128);
		PrintFlashMemory(TAG, 0x3ff000 - 96, 96);
		PrintFlashMemory(TAG, CONFIGURATION_PROGRAM_FLASH_ADDRESS, 128);
		DELAY_MS(100);
	}

// LOGI(TAG, "flashConfigurationBootloader.currentFlashAddress 0x%08x ", flashConfigurationBootloader.currentFlashAddress);

	return STATUS_OK;
}

void mainTask(void *arg)
{
#ifdef WATCHDOG
	esp_task_wdt_add(NULL);
#endif

#ifdef ISR_TEMP
	for (uint8_t i = 0; i < sizeof(DS18B20Temperatures) / sizeof(int16_t); i++)
		DS18B20Temperatures[i] = 0x7fff;
#endif

#ifndef BOOTLOADER
	uint8_t bootState = GetBootState();
#endif
	uint32_t ledTicks;
	uint8_t ledOn = 0;
	while (true)
	{
#ifdef BOOTLOADER
		ledTicks = (GET_MS() - synchronizationDifference) % 100;
#else
		ledTicks = (GET_MS() - synchronizationDifference) % 1000;
#endif
		ledOn = (ledTicks <= 50) ? 1 : 0;
		// ledOn |= (GET_MS() >= ledFastBlink && GET_MS() <= ledFastBlink + 1000 && ((ledTicks / 100) % 2) != 0) ? true : false;
#ifdef ISR_CU
		if (communicationTcp.directModeSource != DIRECT_MODE_SOURCE_OFF)
		{
			ledTicks = (GET_MS() - synchronizationDifference) % 1000;
			ledOn = (ledTicks <= 50) ? 1 : 0;
			ledOn |= (ledTicks >= 300 && ledTicks <= 350) ? 1 : 0;
		}
#endif
		gpio_set_level(GPIO_LED, ledOn);

		uptime = GET_MS() / 1000;

		Adc_Loop();

#ifndef BOOTLOADER
		if ((bootState == CONFIGURATION_BOOT_STATE_PROGRAM_NOT_EXISTS ||
					bootState == CONFIGURATION_BOOT_STATE_PROGRAM_EXISTS) &&
				GET_MS() >= 2000)
		{
			bootState = CONFIGURATION_BOOT_STATE_PROGRAM_EXISTS_AND_RAN;
			SetBootState(bootState);
		}
#endif

#ifdef ISR_CU
			/// Execute TCP server commands
		uint16_t bytesToSend = ExecuteCommunicationCommand(&communicationTcp, communicationTcp.bufferTx, sizeof(communicationTcp.bufferTx));
#ifdef BOOTLOADER
		if (bytesToSend == 0)
			bytesToSend = ExecuteCommunicationCommand_Bootloader(&communicationTcp, communicationTcp.bufferTx, sizeof(communicationTcp.bufferTx));
#endif
		if (bytesToSend > 0)
			TcpServer_Send(communicationTcp.socket, communicationTcp.bufferTx, bytesToSend);

			/// Disable direct mode after few minutes
		if (communicationTcp.directModeSource == DIRECT_MODE_SOURCE_ETHERNET &&
				GET_MS() - communicationTcp.lastReceived >= DIRECT_MODE_MAX_MILLISECONDS)
			communicationTcp.directModeSource = DIRECT_MODE_SOURCE_OFF;
#endif

#ifdef ISR_TEMP
		bool forceMeasure = ((GET_MS() / 1024) % 5) == 0;
		uint16_t ms = GET_MS() % 1024;
		if ((ms == 100) && (forceMeasure || DS18B20Temperatures[0] == 0x7fff))
			DS18B20_Start1(0, TEMP1_GPIO_Port);
		else if ((ms == 120) && (forceMeasure || DS18B20Temperatures[1] == 0x7fff))
			DS18B20_Start1(1, TEMP2_GPIO_Port);
		// else if ((ms == 140) && (forceMeasure || DS18B20Temperatures[2] == 0x7fff))
		// 	DS18B20_Start1(2, TEMP3_GPIO_Port);
		// else if ((ms == 160) && (forceMeasure || DS18B20Temperatures[3] == 0x7fff))
		// 	DS18B20_Start1(3, TEMP4_GPIO_Port);

		else if ((ms == 900) && (forceMeasure || DS18B20Temperatures[0] == 0x7fff))
			DS18B20_Start2(0, TEMP1_GPIO_Port);
		else if ((ms == 920) && (forceMeasure || DS18B20Temperatures[1] == 0x7fff))
			DS18B20_Start2(1, TEMP2_GPIO_Port);
		// else if ((ms == 940) && (forceMeasure || DS18B20Temperatures[2] == 0x7fff))
		// 	DS18B20_Start2(2, TEMP3_GPIO_Port);
		// else if ((ms == 960) && (forceMeasure || DS18B20Temperatures[3] == 0x7fff))
		// 	DS18B20_Start2(3, TEMP4_GPIO_Port);
#endif

#if !defined(BOOTLOADER) && defined(ISR_CU)
		if (writeCuControlTimestamp > 0 && GET_MS() - writeCuControlTimestamp >= 1000)
		{
			DeviceItem_WriteControlToFlash(&flashConfigurationCuControl);
			writeCuControlTimestamp = 0;
		}

		if (writeVisualControlsTimestamp > 0 && GET_MS() - writeVisualControlsTimestamp >= 1000)
		{
			VisualComponent_WriteVisualComponentsToFlash(&flashConfigurationVisualComponents);
			writeVisualControlsTimestamp = 0;
		}
#endif

			/// watchdog
#ifdef WATCHDOG
		if (GET_MS() - lastWatchdogReset >= WATCHDOG_RESET_TIME)
		{
			esp_task_wdt_reset();
			lastWatchdogReset = GET_MS();

#ifdef ISR_CU
			if (LogLevel >= 2)
			{
				char tis[32];
				RTC_GetCurrentDateTimeString(tis, sizeof(tis));
				LOGI(TAG, "system time %s, isset = %d", tis, RTC_isSetRtc);
				LOGI(TAG, "WDT");
			}
#endif
		}
#endif

		DELAY_MS(1);
	}

#ifdef WATCHDOG
	esp_task_wdt_delete(NULL);
#endif
}

#ifdef ISR_CU
void InitializeRTC(void)
{
	RTC_Initialize();
	RTC_DS3231_Initialize();
	struct tm timeinfo;
	memset(&timeinfo, 0, sizeof(timeinfo));
	if (LogLevel >= 2)
	{
		RTC_DS3231_SetTimeInfo(&timeinfo, 2022, 9, 1, 6, 12, 47, 25);
		LOGI(TAG, "ds3231 write err=%d", RTC_DS3231_Write(&timeinfo));
	}
	esp_err_t readRtcOk = RTC_DS3231_Read(&timeinfo);
	if (readRtcOk == STATUS_OK)
		RTC_SetSystemTime(&timeinfo, 0);

	char tis[64];
	RTC_DS3231_GetDateTimeString(&timeinfo, tis, sizeof(tis));
	LOGI(TAG, "ds3231 read err=%d, %s", readRtcOk, tis);

	RTC_GetCurrentDateTimeString(tis, sizeof(tis));
	LOGI(TAG, "system time %s, isset = %d", tis, RTC_isSetRtc);
}
#endif

void app_main()
{
	// LOGI(TAG, "RND %x %x", esp_random(), esp_random());	/// debug: check randomization
	// SetBootState(0);																			/// debug: reset boot state flash
	// EraseFlashBootloader();															/// debug: reset bootloader flash
	// EraseFlashProgram();																	/// debug: reset program flash
	// EraseFlashCuControl();																/// debug: reset cu control flash
	// EraseFlashVisualComponents();												/// debug: reset visual components flash

	ProgramIndicator[0] = 0x57;

	LOGI(TAG, "App started.");

		/// Initialize watchdog
#ifdef WATCHDOG
	esp_task_wdt_init(5, true);
#endif

	ESP_ERROR_CHECK(nvs_flash_init());

#ifdef ISR_CU
	InitializeRTC();
#endif
#if !defined(BOOTLOADER) && defined(ISR_CU)
	DeviceItem_Initialize();
	VisualComponent_Initialize();
#endif

	InitilizeConfiguration();

#if !defined(BOOTLOADER) && defined(ISR_CU)
	bool ok = DeviceItem_LoadControlFromFlash(&flashConfigurationCuControl);
	if (LogLevel >= 1)
		LOGI(TAG, "DeviceItem_LoadControlFromFlash ok = %d, cfg size = %d", (uint8_t)ok, flashConfigurationCuControl.configurationSize);

	ok = VisualComponent_LoadVisualComponentsFromFlash(&flashConfigurationVisualComponents);
	if (LogLevel >= 1)
		LOGI(TAG, "VisualComponent_LoadVisualComponentsFromFlash ok = %d, cfg size = %d", (uint8_t)ok, flashConfigurationVisualComponents.configurationSize);
#endif

	LOGI(TAG, "bootState: %d ", GetBootState());
	if (LogLevel >= 2)
	{
		PrintFlashMemory(TAG, CONFIGURATION_PROGRAM_FLASH_ADDRESS, 128);
		PrintFlashMemory(TAG, 0x10000, 128);
		PrintFlashMemory(TAG, 0x80000, 128);
	}

	DELAY_MS(20);
	Adc_Initialize();
#ifdef ISR_CU
	InitializeUart1();
	InitializeUart2();
#endif
#ifdef ISR_TEMP
	InitializeUart2();
	InitializeTemperatures();
	InitializeTimer();
#endif
	InitializeLed();
#ifdef ISR_CU
	InitializeCommunication(&communicationTcp, LINE_LAN);
	Lan8720a_Initialize();
#endif

	isInitialized = true;

	LOGI(TAG, "App start loop.");

// 		Erase flash
// int32_t i = 0x110000;
// while (i < 0x138000)
// {
// 	LOGI(TAG, " 0x%x ", i);
// 	esp_flash_erase_region(esp_flash_default_chip, i, 0x1000);
// DELAY_MS(10);
// 	i += 0x1000;
// }

	xTaskCreate(mainTask, "MAIN_TASK", 4096, NULL, 12, NULL);
}

#ifdef ISR_TEMP
void DS18B20_MeasureCallback(uint8_t number, int16_t temperature)
{
	DS18B20Temperatures[number] = temperature;
}
#endif

#ifdef ISR_CU
void Lan8720a_EthEventHandler(void *arg, esp_event_base_t event_base,
		int32_t event_id, void *event_data)
{
}

static bool rtcInitialized = false;

void Lan8720a_GotIpEventHandler(void *arg, esp_event_base_t event_base,
		int32_t event_id, void *event_data)
{
	TcpServer_CreateServer();
	TcpClient_CreateClient();
	if (!rtcInitialized)
	{
		RTC_InitializeSntp();
		rtcInitialized = true;
	}
}

uint16_t TcpServer_AnswerForRequest(int32_t socket, uint8_t* rxBuffer, uint16_t rxBufferLength,
		uint8_t* txBuffer, uint16_t txBufferLength)
{
	uint16_t bytesToSend = 0;
	uint16_t rxBufferIndex = rxBufferLength;
	bool isAnswer;
	uint16_t bytesReceivedCorrect = CheckFrameInBuffer(&communicationTcp, rxBuffer, &rxBufferIndex,
			BROADCAST, cfgBootloader.deviceAddress, &isAnswer);
	if (bytesReceivedCorrect > 0)
	{
		communicationTcp.lastReceived = GET_MS();
		communicationTcp.socket = socket;

		bytesToSend = ReceiveForRequest(&communicationTcp, bytesReceivedCorrect, rxBuffer, txBuffer, txBufferLength,
				PACKET_SOURCE_ETHERNET);
#ifdef BOOTLOADER
		if (bytesToSend == 0)
			bytesToSend = ReceiveForRequest_Bootloader(&communicationTcp, bytesReceivedCorrect, rxBuffer,
					txBuffer, txBufferLength);
#endif
	}

	if (LogLevel >= 1)
		LOGI(TAG, "read from ethernet to direct mode source: %d %d", rxBufferLength, communicationTcp.directModeSource);

	if (bytesToSend == 0 && communicationTcp.directModeSource == DIRECT_MODE_SOURCE_ETHERNET)
	{
		communicationTcp.lastSent = GET_MS();
		if (communicationTcp.directModeTarget == DIRECT_MODE_TARGET_ALL ||
				communicationTcp.directModeTarget == DIRECT_MODE_TARGET_UART1)
		{
			memcpy(communication1.bufferTx, rxBuffer, rxBufferLength);
			Uart1Send(communication1.bufferTx, rxBufferLength);
		}
		if (communicationTcp.directModeTarget == DIRECT_MODE_TARGET_ALL ||
				communicationTcp.directModeTarget == DIRECT_MODE_TARGET_UART2)
		{
			memcpy(communication2.bufferTx, rxBuffer, rxBufferLength);
			Uart2Send(communication2.bufferTx, rxBufferLength);
		}
	}

	if (bytesToSend > 0)
		communicationTcp.lastSent = GET_MS();
	return bytesToSend;
}
#endif

#if !defined(BOOTLOADER) && defined(ISR_CU)
void Command_AnswerGetRelaysStatus(uint32_t address, bool answerOk, uint32_t uptime, uint16_t vin, bool *relays, uint8_t count)
{
	if (LogLevel >= 1)
	{
		if (answerOk) LOGI("COMMAND", "get rel: %d, %d: %d %d", (uint8_t)answerOk, count, relays[0], relays[1]);
		else          LOGI("COMMAND", "get rel: %d, %d", (uint8_t)answerOk, count);
	}

	struct DeviceItemStatus *deviceItemStatus;
	deviceItemStatus = NULL;
	if (DeviceItem_GetDeviceStatusByAddress(address, &deviceItemStatus))
	{
		struct RelayStatus *relay;
		relay = (struct RelayStatus*)deviceItemStatus->status;
		for (uint8_t i = 0; i < MIN(deviceItemStatus->deviceItem->hardwareSegmentsCount, count); i++)
		{
			if (answerOk)
				relay->relayState = relays[i];
			relay++;
		}

		if (!answerOk && (!deviceItemStatus->error || deviceItemStatus->errorFrom == 0))
			deviceItemStatus->errorFrom = GET_MS();
		deviceItemStatus->error = !answerOk;
		deviceItemStatus->initialized = true;
		deviceItemStatus->uptime = uptime;
		deviceItemStatus->vin = vin;

		if (LogLevel >= 1)
		{
			struct RelayStatus *relay1; struct RelayStatus *relay2;
			relay1 = (struct RelayStatus*)deviceItemStatus->status;
			relay2 = (struct RelayStatus*)deviceItemStatus->status + 1;
			LOGI("COMMAND", "  relay: err1: %d init1: %d state1: %d, err2: %d init2: %d state2: %d",
					deviceItemStatus->error, deviceItemStatus->initialized, relay1->relayState,
					deviceItemStatus->error, deviceItemStatus->initialized, relay2->relayState);
		}
	}
}

void Command_AnswerSetRelaysStatus(uint32_t address, bool answerOk)
{
	if (LogLevel >= 1)
		LOGI("COMMAND", "set rel: %d", (uint8_t)answerOk);
}

void Command_AnswerGetTemperatures(uint32_t address, bool answerOk, uint32_t uptime, uint16_t vin, uint16_t *temperatures, uint8_t count)
{
	if (LogLevel >= 1)
	{
		if (answerOk) LOGI("COMMAND", "get temp: %d, %d: %.2f, %.2f, %.2f, %.2f",
											(uint8_t)answerOk, count, (float)temperatures[0] / 16, (float)temperatures[1] / 16,
											(float)temperatures[2] / 16, (float)temperatures[3] / 16);
		else          LOGI("COMMAND", "get temp: %d, %d", (uint8_t)answerOk, count);
	}

	struct DeviceItemStatus *deviceItemStatus;
	deviceItemStatus = NULL;
	if (DeviceItem_GetDeviceStatusByAddress(address, &deviceItemStatus))
	{
		struct TemperatureStatus *temperature;
		temperature = (struct TemperatureStatus*)deviceItemStatus->status;
		for (uint8_t i = 0; i < MIN(deviceItemStatus->deviceItem->hardwareSegmentsCount, count); i++)
		{
			if (answerOk)
			{
				if (!temperature->error && temperatures[i] == 0x7fff)
					temperature->errorFrom = GET_MS();
				temperature->error = temperatures[i] == 0x7fff;
				temperature->temperature = temperatures[i];
			}
			temperature++;
		}

		if (!answerOk && (!deviceItemStatus->error || deviceItemStatus->errorFrom == 0))
			deviceItemStatus->errorFrom = GET_MS();
		deviceItemStatus->error = !answerOk;
		deviceItemStatus->initialized = true;
		deviceItemStatus->uptime = uptime;
		deviceItemStatus->vin = vin;

		if (LogLevel >= 1)
		{
			struct TemperatureStatus *temperature1; struct TemperatureStatus *temperature2;
			struct TemperatureStatus *temperature3; struct TemperatureStatus *temperature4;
			temperature1 = (struct TemperatureStatus*)deviceItemStatus->status;
			temperature2 = (struct TemperatureStatus*)deviceItemStatus->status + 1;
			temperature3 = (struct TemperatureStatus*)deviceItemStatus->status + 2;
			temperature4 = (struct TemperatureStatus*)deviceItemStatus->status + 3;
			LOGI("COMMAND", "  temperature: %d %d 0x%.4x, %d %d 0x%.4x, %d %d 0x%.4x, %d %d 0x%.4x",
					deviceItemStatus->error, deviceItemStatus->initialized, temperature1->temperature,
					deviceItemStatus->error, deviceItemStatus->initialized, temperature2->temperature,
					deviceItemStatus->error, deviceItemStatus->initialized, temperature3->temperature,
					deviceItemStatus->error, deviceItemStatus->initialized, temperature4->temperature);
		}
	}
}
#endif


// portDISABLE_INTERRUPTS() portENABLE_INTERRUPTS()
// vTaskSuspendAll() xTaskResumeAll()
// tskIDLE_PRIORITY, 0 to (configMAX_PRIORITIES – 1), configMAX_PRIORITIES = 25
// portNUM_PROCESSORS
// portTICK_PERIOD_MS pdMS_TO_TICKS() pdTICKS_TO_MS()
// (portTickType)portMAX_DELAY)
// uint8_t *data = (uint8_t*)malloc(BUF_SIZE);
// free(data);

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/portable.h"
// int stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
// int freeHeapSize = xPortGetFreeHeapSize();
// LOGI(TAG, "stackHighWaterMark %d, freeHeapSize %d, uptime %d s", stackHighWaterMark, freeHeapSize, GET_MS() / 1000);

// C:\Users\jarsul\.platformio\packages\framework-espidf\components\freertos\include\esp_additions\freertos\FreeRTOSConfig.h

/*
	Build, uplad and monitor:
		Ctrl + Shift + P: PlatformIO: New Terminal
		Ctrl + `
		pio run --target upload && pio device monitor

	Disable UART log messages at startup - bootloader:
		pio run -t menuconfig
			Compiler options > Optimization Level > Optimize for performance (-O2)
			Bootloader config > Bootloader log verbosity > (no output)
			Component config > Log output > Default log verbosity > (no output)
				sdkconfig:
					CONFIG_LOG_DEFAULT_LEVEL_NONE=y
					CONFIG_LOG_MAXIMUM_LEVEL=0
		GPIO15 pull down to ground
	Disable UART log messages in program:
		esp_log_level_set("*", ESP_LOG_NONE);

	sdkconfig:
		CONFIG_FREERTOS_HZ=1000

	Partitions:
		cd %USERPROFILE%\.platformio\packages\framework-espidf\components\partition_table
		python gen_esp32part.py --verify partitions.csv partitions.bin
		copy partitions.bin to /.pio/build/az-delivery-devkit-v4/partitions.bin

	Fix bootloader:
		Copy from \bootloader:
			%USERPROFILE%\.platformio\packages\framework-espidf\components\bootloader_support\include_bootloader\isr_bootloader.h
			%USERPROFILE%\.platformio\packages\framework-espidf\components\bootloader_support\src\isr_bootloader.c

		%USERPROFILE%\.platformio\packages\framework-espidf\components\bootloader_support\CMakeLists.txt
			set(srcs
			...
			"src/isr_bootloader.c"

		%USERPROFILE%\.platformio\packages\framework-espidf\components\bootloader\subproject\main\bootloader_start.c
			#include "isr_bootloader.h"  /// jarsulk
			...
			// before 3. Load the app image for booting
			boot_index = GetActivePartitionFromBootState();   /// jarsulk

	BootState variable:
		1. ESP Bootloader:     if BootState == 0 -> jump to Program Bootloader
		2. Program Bootloader: BootState == 0, programming, BootState = 1, reset
		3. ESP Bootloader:     if BootState == 1 -> BootState == 0, jump to Program
		4. Program:            if BootState == (0 || 1) -> BootState = 2

	CPU: ESP32-D0WD
		SRAM: 520KB
		External FLASH: 4MB
		Clock: 160MHz (80 - 240MHz)

	Upload bootloader to flash:
set dir="D:\jarsul\Projects\Inteligentny_dom\Programy\ESP32\ISR-ESP32-BOOTLOADER"
cd /D %dir%
set PYTHONPATH=PYTHONPATH	C:\Users\jarsul\.platformio\penv\Scripts;C:\Users\jarsul\.platformio\python3\python39.zip;C:\Users\jarsul\.platformio\python3\DLLs;C:\Users\jarsul\.platformio\python3\lib;C:\Users\jarsul\.platformio\python3;C:\Users\jarsul\.platformio\penv;C:\Users\jarsul\.platformio\penv\lib\site-packages

set com="COM4"
set bins_dir="D:\jarsul\Projects\Inteligentny_dom\Programy\ESP32\Programs bins"
set python="C:\Users\jarsul\.platformio\python3\python.exe"
set tool="C:\Users\jarsul\.platformio\packages\tool-esptoolpy\esptool.py"
set tool_params=--chip esp32 --port %com% --baud 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size 4MB
set partitions=%bins_dir%\partitions_1.0.bin
set bootloader=%bins_dir%\bootloader_1.0.bin

:: ISR-DIN-CU:
set bootloader2=%bins_dir%\Bootloader_ISR-DIN-CU_1.0.bin
set firmware=%bins_dir%\ISR-DIN-CU_1.2.bin
:: ISR-BOX-TEMP-2:
set bootloader2=%bins_dir%\Bootloader_ISR-BOX-TEMP-2_1.0.bin
set firmware=%bins_dir%\ISR-BOX-TEMP-2_1.0.bin

:: only partitions, bootloader
%python% %tool% %tool_params% 0x1000 %bootloader% 0x8000 %partitions%
:: partitions, bootloader, bootloader2
%python% %tool% %tool_params% 0x1000 %bootloader% 0x8000 %partitions% 0x10000 %bootloader2%
:: firmware
%python% %tool% %tool_params% 0xc0000 %firmware%

	todo:
		ESPNOW
			https://github.com/espressif/esp-idf/tree/1067b28707e527f177752741e3aa08b5dc64a4d7/examples/wifi/espnow
*/
