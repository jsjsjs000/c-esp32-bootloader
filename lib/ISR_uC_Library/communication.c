#include "../../include/main.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#ifdef ESP32
	#include <time.h>
	#include "freertos/FreeRTOS.h"
	#include "freertos/task.h"
	#include "esp_system.h"
	#include "esp_task_wdt.h"
	#include "esp_log.h"
	#include "esp_err.h"
	#include "../ISR_ESP32_Library/adc.h"
	#include "../ISR_ESP32_Library/flash.h"
	#include "../ISR_ESP32_Library/rtc_ds3231.h"
	#include "../ISR_ESP32_Library/rtc.h"
#endif
#ifdef STM32F0
	#include "stm32f0xx_hal.h"
	#include "stm32f0xx_init.h"
#endif
#ifdef STM32F4
	#include "stm32f4xx_hal.h"
	#include "stm32f4xx_init.h"
#endif
#if defined(STM32F0) || defined(STM32F4)
	#include "../ISR_STM32_Library/adc.h"
	#include "../ISR_STM32_Library/flash.h"
	#include "../ISR_STM32_Library/rnd.h"
	#include "../ISR_STM32_Library/rtc.h"
#endif
#include "common.h"
#include "configuration.h"
#include "configuration_boot_state.h"
#include "configuration_bootloader.h"
#include "configuration_program.h"
#include "packets.h"
#ifdef BOOTLOADER
	#include "communication_bootloader.h"
#endif
#include "communication.h"
#ifdef ISR_CU
	#include "commands.h"
	#include "device_item2.h"
	#include "device_item.h"
#endif

void InitializeCommunication(struct Communication *communication, uint8_t lineNumber)
{
	communication->lineNumber = lineNumber;

	communication->command = COMMUNICATION_COMMAND_NONE;
	communication->commandFA_blockTime = 0;
	communication->commandFA_replayDelay = 0;
	communication->commandStatus = STATUS_OK;
	communication->commandPagesToClear = 0;
	communication->commandProgramCrc32 = 0;
	communication->commandSetHardwareType1 = CONFIGURATION_HARDWARE_TYPE1_NONE;
	communication->commandSetHardwareType2 = CONFIGURATION_HARDWARE_TYPE2_NONE;
	communication->commandSetHardwareSegmentsCount = 0;
	communication->commandSetHardwareVersion = 0;
	communication->commandSetAddress = 0;
	communication->lastSent = 0;
	communication->lastReceived = 0;
	communication->socket = 0;
	communication->directModeSource = DIRECT_MODE_SOURCE_OFF;
	communication->directModeTarget = DIRECT_MODE_TARGET_ALL;
	communication->lastPacketSource = PACKET_SOURCE_NONE;
	communication->packetId = 0;
	communication->encryptionKey = 0;
	communication->address = 0;

#ifdef ESP32
	communication->bufferRxIndex = 0;
#endif

#ifdef ISR_CU
	communication->sendCommand = SEND_COMMAND_NONE;
	communication->sendCommandAddress = BROADCAST;
	communication->sendCommandBytesToSend = 0;
	communication->sendCommandSegmentsCount = 0;
	communication->lastSendCommandTime = 0;
	communication->lastSendCommandPacketId = 0;
	communication->lastSendCommandEncryptionKey = 0;
	communication->lastSendCommandAddress = 0;
#endif
}

uint16_t ExecuteCommunicationCommand(struct Communication *communication, uint8_t *txBuffer, uint16_t txBufferLength)
{
	uint16_t bytesToSend = 0;
	uint8_t* txBuffer_ = (uint8_t*)(txBuffer + PACKET_PRE_BYTES);
	uint16_t j = 0;

		/// 'RESET' - Reset Device
	if (communication->command == COMMUNICATION_COMMAND_RESET)
	{
		communication->command = COMMUNICATION_COMMAND_NONE;
		DELAY_MS(30);																				/// wait for send answer
		ResetDevice();
	}

		/// 0xfa 1 - Get Device Address with random delay
	else if (communication->command == COMMUNICATION_COMMAND_GET_ADDRESS_WITH_RANDOM_DELAY &&
			GET_MS() >= communication->commandFA_replayDelay)
	{
		communication->command = COMMUNICATION_COMMAND_NONE;

		txBuffer_[j++] = 0xfa;
		txBuffer_[j++] = UINT32_3BYTE(cfgBootloader.deviceAddress);
		txBuffer_[j++] = UINT32_2BYTE(cfgBootloader.deviceAddress);
		txBuffer_[j++] = UINT32_1BYTE(cfgBootloader.deviceAddress);
		txBuffer_[j++] = UINT32_0BYTE(cfgBootloader.deviceAddress);
		txBuffer_[j++] = cfgBootloader.hardwareType1;
		txBuffer_[j++] = cfgBootloader.hardwareType2;
		txBuffer_[j++] = cfgBootloader.hardwareSegmentsCount;
		txBuffer_[j++] = cfgBootloader.hardwareVersion;
		bytesToSend = j;
	}

		/// 0xfb 1 - Direct Mode On
	else if (communication->command == COMMUNICATION_COMMAND_DIRECT_MODE_ON)
	{
		communication->command = COMMUNICATION_COMMAND_NONE;
		DELAY_MS(DIRECT_MODE_DELAY);         /// wait for answers from devices on sensors lines
		communication->directModeSource = communication->commandParam1;
		communication->directModeTarget = communication->commandParam2;
	}

		/// 0xfb 0 - Direct Mode Off
	else if (communication->command == COMMUNICATION_COMMAND_DIRECT_MODE_OFF)
	{
		communication->command = COMMUNICATION_COMMAND_NONE;
		DELAY_MS(DIRECT_MODE_DELAY);         /// wait for answers from devices on sensors lines
		communication->directModeSource = DIRECT_MODE_SOURCE_OFF;
		communication->directModeTarget = DIRECT_MODE_TARGET_ALL;
	}

	if (bytesToSend == 0)
		return 0;

	return EncodePacket(txBuffer, txBufferLength, communication->packetId, 0x08080808, cfgBootloader.deviceAddress, // $$
			true, bytesToSend);
}

uint16_t ReceiveForRequest(struct Communication *communication, uint16_t receivedBytes, uint8_t *rxBuffer,
		uint8_t *txBuffer, uint16_t txBufferLength, uint8_t packetSource)
{
	uint16_t bytesToSend = 0;
	uint8_t* rxBuffer_ = (uint8_t*)(rxBuffer + PACKET_PRE_BYTES);
	uint8_t* txBuffer_ = (uint8_t*)(txBuffer + PACKET_PRE_BYTES);
	uint32_t address = UINT32_FROM_ARRAY_BE(rxBuffer, 11);
	uint16_t j = 0;

#ifdef ISR_CU
		/// 0xfb - Get Direct Mode
	if (receivedBytes == 1 && rxBuffer_[0] == 0xfb && address != BROADCAST)
	{
		txBuffer_[j++] = rxBuffer_[0];
		txBuffer_[j++] = 0; /// for answer ambiguity
		txBuffer_[j++] = communication->directModeSource;
		bytesToSend = j;
	}
		/// 0xfb - Set Direct Mode
	else if (receivedBytes == 2 && rxBuffer_[0] == 0xfb && address != BROADCAST)
	{
		uint8_t line = rxBuffer_[1];
		if (line != 0)
			communication->command = COMMUNICATION_COMMAND_DIRECT_MODE_ON;
		else
			communication->command = COMMUNICATION_COMMAND_DIRECT_MODE_OFF;
		communication->commandParam1 = packetSource;
		communication->commandParam2 = line;
		txBuffer_[j++] = rxBuffer_[0];
		txBuffer_[j++] = 0;
		bytesToSend = j;
	}
	else if (communication->directModeSource != DIRECT_MODE_SOURCE_OFF)
		return 0;
#endif

		/// 'v' - Get Device Program, Bootloader Version, Date Programming And Device Serial Number
	if (receivedBytes == 1 && rxBuffer_[0] == (char)'v')
	{
		ledFastBlink = GET_MS();

		txBuffer_[j++] = (char)'v';
		txBuffer_[j++] = cfgBootloader.programedProgramYear;
		txBuffer_[j++] = cfgBootloader.programedProgramMonth;
		txBuffer_[j++] = cfgBootloader.programedProgramDay;
		txBuffer_[j++] = cfgBootloader.programedProgramHour;
		txBuffer_[j++] = cfgBootloader.programedProgramMinute;
#ifdef BOOTLOADER
		txBuffer_[j++] = cfgBootloader.programYear;
		txBuffer_[j++] = cfgBootloader.programMonth;
		txBuffer_[j++] = cfgBootloader.programDay;
		txBuffer_[j++] = cfgBootloader.programVersionMajor;
		txBuffer_[j++] = cfgBootloader.programVersionMinor;
		txBuffer_[j++] = BOOTLOADER_YEAR;
		txBuffer_[j++] = BOOTLOADER_MONTH;
		txBuffer_[j++] = BOOTLOADER_DAY;
		txBuffer_[j++] = BOOTLOADER_VERSION_MAJOR;
		txBuffer_[j++] = BOOTLOADER_VERSION_MINOR;
#else
		txBuffer_[j++] = PROGRAM_YEAR;
		txBuffer_[j++] = PROGRAM_MONTH;
		txBuffer_[j++] = PROGRAM_DAY;
		txBuffer_[j++] = PROGRAM_VERSION_MAJOR;
		txBuffer_[j++] = PROGRAM_VERSION_MINOR;
		txBuffer_[j++] = cfgBootloader.bootloaderYear;
		txBuffer_[j++] = cfgBootloader.bootloaderMonth;
		txBuffer_[j++] = cfgBootloader.bootloaderDay;
		txBuffer_[j++] = cfgBootloader.bootloaderVersionMajor;
		txBuffer_[j++] = cfgBootloader.bootloaderVersionMinor;
#endif
		txBuffer_[j++] = cfgBootloader.hardwareType1;
		txBuffer_[j++] = cfgBootloader.hardwareType2;
		txBuffer_[j++] = cfgBootloader.hardwareSegmentsCount;
		txBuffer_[j++] = cfgBootloader.hardwareVersion;
		txBuffer_[j++] = (uptime >> 24) & 0xff;
		txBuffer_[j++] = (uptime >> 16) & 0xff;
		txBuffer_[j++] = (uptime >> 8) & 0xff;
		txBuffer_[j++] = uptime & 0xff;
		txBuffer_[j++] = (vin >> 8) & 0xff;
		txBuffer_[j++] = vin & 0xff;
#ifdef BOOTLOADER
		txBuffer_[j++] = (char)'b';
#else
		txBuffer_[j++] = (char)'p';
#endif
		bytesToSend = j;
	}

		/// 0xf0 - Clear Device Program
	else if (receivedBytes == 13 && rxBuffer_[0] == 0xf0 && address != BROADCAST &&
			rxBuffer_[1] == (char)'C' && rxBuffer_[2] == (char)'l' &&
			rxBuffer_[3] == (char)'e' && rxBuffer_[4] == (char)'a' &&
			rxBuffer_[5] == (char)'r' && rxBuffer_[6] == (char)'P' &&
			rxBuffer_[7] == (char)'r' && rxBuffer_[8] == (char)'o' &&
			rxBuffer_[9] == (char)'g' && rxBuffer_[10] == (char)'r' &&
			rxBuffer_[11] == (char)'a' && rxBuffer_[12] == (char)'m')
	{
		InitializeConfigurationBootloader(&cfgBootloader, false);
#ifdef ESP32
		StatusType status = SetBootState(CONFIGURATION_BOOT_STATE_PROGRAM_NOT_EXISTS) ? STATUS_OK : STATUS_ERROR;
#endif
#ifndef ESP32
		cfgBootloader.bootState = CONFIGURATION_BOOT_STATE_PROGRAM_NOT_EXISTS;
		StatusType status = WriteConfigurationToFlash(&flashConfigurationBootloader);
#endif

#ifdef BOOTLOADER
		ResetResolvedPackets();
#else
		communication->command = COMMUNICATION_COMMAND_RESET;
#endif

		txBuffer_[j++] = 0xf0;
		txBuffer_[j++] = status != STATUS_OK;
		bytesToSend = j;
	}

		/// 0xfa - Get Device Address
	else if (receivedBytes == 2 && rxBuffer_[0] == 0xfa)
	{
		if (communication->commandFA_blockTime == 0 || GET_MS() - communication->commandFA_blockTime >= 3000)
		{
			if (rxBuffer_[1] == 1)
			{
				communication->commandFA_replayDelay = GET_MS() + RANDOM_TO(1000);
				communication->command = COMMUNICATION_COMMAND_GET_ADDRESS_WITH_RANDOM_DELAY;
			}
			if (rxBuffer_[1] == 2)
				communication->commandFA_blockTime = GET_MS(); // $$ + 3000

			if (rxBuffer_[1] == 0 || rxBuffer_[1] == 2)
			{
				txBuffer_[j++] = 0xfa;
				txBuffer_[j++] = UINT32_3BYTE(cfgBootloader.deviceAddress);
				txBuffer_[j++] = UINT32_2BYTE(cfgBootloader.deviceAddress);
				txBuffer_[j++] = UINT32_1BYTE(cfgBootloader.deviceAddress);
				txBuffer_[j++] = UINT32_0BYTE(cfgBootloader.deviceAddress);
				txBuffer_[j++] = cfgBootloader.hardwareType1;
				txBuffer_[j++] = cfgBootloader.hardwareType2;
				txBuffer_[j++] = cfgBootloader.hardwareSegmentsCount;
				txBuffer_[j++] = cfgBootloader.hardwareVersion;
				bytesToSend = j;
			}
		}
	}

		/// 0xfc - Get Flash Memory
	else if (receivedBytes == 7 && rxBuffer_[0] == 0xfc)
	{
		uint32_t address_ = UINT32_FROM_ARRAY_BE(rxBuffer_, 1);
		uint16_t length = UINT16_FROM_ARRAY_BE(rxBuffer_, 1 + 4);
		txBuffer_[j++] = 0xfc;
		if (length == 0 || length > 256)
			txBuffer_[j++] = 1;
		else
		{
			uint8_t *buffer = (uint8_t*)(txBuffer_ + 2);
			StatusType status = ReadFromFlash(address_, length, buffer);
			txBuffer_[j++] = status != STATUS_OK;
			j += length;
		}
		bytesToSend = j;
	}

		/// 'RESET' - Reset Device
	else if (receivedBytes == 2 && rxBuffer_[0] == 0xff)
	{
		txBuffer_[j++] = 0xff;
		txBuffer_[j++] = 0x00;
		bytesToSend = j;

		communication->command = COMMUNICATION_COMMAND_RESET;
	}

		/// 'p' - Ping
	else if (receivedBytes >= 1 && rxBuffer_[0] == (char)'p')
	{
		for (uint16_t i = 0; i < receivedBytes; i++)
			txBuffer_[i] = rxBuffer_[i];
		bytesToSend = receivedBytes;
	}

		/// 'S' - Send Synchronization
	else if (receivedBytes >= 10 && rxBuffer_[0] == (char)'S')
	{
#if defined(STM32F0) || defined(STM32F4)
		uint8_t year = rxBuffer_[1];
		uint8_t month = rxBuffer_[2];
		uint8_t day = rxBuffer_[3];
		uint8_t dayOfWeek = rxBuffer_[4];
		uint8_t hours = rxBuffer_[5];
		uint8_t minutes = rxBuffer_[6];
		uint8_t seconds = rxBuffer_[7];
		// uint16_t milliseconds = (rxBuffer_[8] << 8) | rxBuffer_[9];

		htim1.Instance->CNT = htim1.Instance->ARR;  /// reset TIM1
		htim3.Instance->CNT = htim3.Instance->ARR;  /// reset TIM3

		StatusType status = RTC_SetDateTime(year, month, day, dayOfWeek, hours, minutes, seconds);
#endif
#ifdef ESP32
		struct tm timeinfo;
		memset(&timeinfo, 0, sizeof(timeinfo));
		timeinfo.tm_year = rxBuffer_[1] + 2000;
		timeinfo.tm_mon = rxBuffer_[2];
		timeinfo.tm_mday = rxBuffer_[3];
		timeinfo.tm_wday = rxBuffer_[4];
		timeinfo.tm_hour = rxBuffer_[5];
		timeinfo.tm_min = rxBuffer_[6];
		timeinfo.tm_sec = rxBuffer_[7];
		uint16_t milliseconds = (rxBuffer_[8] << 8) | rxBuffer_[9];

		RTC_SetSystemTime(&timeinfo, milliseconds * 1000);

		StatusType status = STATUS_OK;
#ifdef ISR_CU
		status = RTC_DS3231_Write(&timeinfo);
		// if (status == STATUS_OK)
		// 	status = RTC_DS3231_Read(&timeinfo);	/// too long (1s) if module not found
#endif
#endif
#if defined(ESP32) && !defined(ISR_CU)
		synchronizationDifference = GET_MS() % 1000;
#endif

		if (communication->address != BROADCAST)
		{
			txBuffer_[j++] = (char)'S';
			txBuffer_[j++] = (status == STATUS_OK) ? 0 : 1;
			bytesToSend = j;
		}
	}

#if !defined(BOOTLOADER) && defined(ISR_REL)
		/// 'r' - Get Relays Status
	else if (receivedBytes == 1 && rxBuffer_[0] == (char)'r')
	{
		txBuffer_[j++] = rxBuffer_[0];
		txBuffer_[j++] = (uptime >> 24) & 0xff;
		txBuffer_[j++] = (uptime >> 16) & 0xff;
		txBuffer_[j++] = (uptime >> 8) & 0xff;
		txBuffer_[j++] = uptime & 0xff;
		txBuffer_[j++] = (vin >> 8) & 0xff;
		txBuffer_[j++] = vin & 0xff;
		txBuffer_[j++] = sizeof(relays);
		for (uint8_t i = 0; i < sizeof(relays); i++)
			txBuffer_[j++] = relays[i];
		bytesToSend = j;
	}

		/// 'r' - Set Relay State
	else if (receivedBytes == 3 && rxBuffer_[0] == (char)'r')
	{
		uint8_t relayNumber = rxBuffer_[1];
		bool ok = relayNumber <= ISR_SEGMENTS_COUNT;
		if (ok)
			relays[relayNumber] = rxBuffer_[2] != 0;
		txBuffer_[j++] = rxBuffer_[0];
		txBuffer_[j++] = ok ? 1 : 0;
		bytesToSend = j;
	}
#endif
#if !defined(BOOTLOADER) && defined(ISR_TEMP)
		/// 't' - Temperature
	else if (receivedBytes == 1 && rxBuffer_[0] == (char)'t')
	{
		txBuffer_[j++] = rxBuffer_[0];
		txBuffer_[j++] = (uptime >> 24) & 0xff;
		txBuffer_[j++] = (uptime >> 16) & 0xff;
		txBuffer_[j++] = (uptime >> 8) & 0xff;
		txBuffer_[j++] = uptime & 0xff;
		txBuffer_[j++] = (vin >> 8) & 0xff;
		txBuffer_[j++] = vin & 0xff;
		txBuffer_[j++] = sizeof(DS18B20Temperatures) / sizeof(int16_t);
		for (uint8_t i = 0; i < sizeof(DS18B20Temperatures) / sizeof(int16_t); i++)
		{
			txBuffer_[j++] = UINT32_1BYTE(DS18B20Temperatures[i]);
			txBuffer_[j++] = UINT32_0BYTE(DS18B20Temperatures[i]);
		}
		bytesToSend = j;
	}
#endif

#if !defined(BOOTLOADER) && defined(ISR_CU)
		/// 'g' - Get Central Unit Status
	else if (receivedBytes == 4 && rxBuffer_[0] == (char)'g')
	{
		uint16_t fromItem = (rxBuffer_[1] >> 8) | rxBuffer_[2];
		uint8_t details = rxBuffer_[3];
		txBuffer_[j++] = rxBuffer_[0];
		j += DeviceItem_GetStatus(&devicesItems, devicesItemsStatus, heatingDevicesComponents,
				&txBuffer_[j], 256 - PACKET_PRE_BYTES - PACKET_POST_BYTES, fromItem, details);
		bytesToSend = j;
	}

		/// "sREL" - Set Relay State
	else if (receivedBytes == 10 && rxBuffer_[0] == (char)'s' &&
			rxBuffer_[1] == (char)'R' && rxBuffer_[2] == (char)'E' && rxBuffer_[3] == (char)'L')
	{
		uint32_t address_ = UINT32_FROM_BYTES(rxBuffer_[4], rxBuffer_[5], rxBuffer_[6], rxBuffer_[7]);
		uint8_t relay = rxBuffer_[8];
		bool state = rxBuffer_[9] != 0;
		struct DeviceItem *deviceItem;
		deviceItem = NULL;
		bool ok = DeviceItem_GetDeviceItemFromAddress(address_, &deviceItem);
		if (ok)
			ok = deviceItem->hardwareType2 == CONFIGURATION_HARDWARE_TYPE2_REL;
		if (ok)
		{
			struct RelayControl *relayControl;
			relayControl = deviceItem->deviceItemControl;
			ok = false;
			for (uint8_t i = 0; i < deviceItem->hardwareSegmentsCount; i++)
			{
				if (i == relay)
				{
					relayControl->setTime = GET_MS();
					relayControl->updateTime = 0;
					relayControl->relayState = state;
					ok = true;
					writeCuControlTimestamp = GET_MS();
					break;
				}
				relayControl++;
			}
		}

		txBuffer_[j++] = (char)'s';
		txBuffer_[j++] = (char)'R';
		txBuffer_[j++] = (char)'E';
		txBuffer_[j++] = (char)'L';
		txBuffer_[j++] = ok ? 0 : 1;
		bytesToSend = j;
	}

		/// "gREL" - Get Relay State
	else if (receivedBytes == 9 && rxBuffer_[0] == (char)'g' &&
			rxBuffer_[1] == (char)'R' && rxBuffer_[2] == (char)'E' && rxBuffer_[3] == (char)'L')
	{
		uint32_t address_ = UINT32_FROM_BYTES(rxBuffer_[4], rxBuffer_[5], rxBuffer_[6], rxBuffer_[7]);
		uint8_t relay = rxBuffer_[8];
		uint8_t state = 2;
		struct DeviceItem *deviceItem;
		deviceItem = NULL;
		bool ok = DeviceItem_GetDeviceItemFromAddress(address_, &deviceItem);
		if (ok)
			ok = deviceItem->hardwareType2 == CONFIGURATION_HARDWARE_TYPE2_REL;
		if (ok)
		{
			struct RelayControl *relayControl;
			relayControl = deviceItem->deviceItemControl;
			for (uint8_t i = 0; i < deviceItem->hardwareSegmentsCount; i++)
			{
				if (i == relay)
				{
					state = relayControl->relayState;
					break;
				}
				relayControl++;
			}
		}

		txBuffer_[j++] = (char)'g';
		txBuffer_[j++] = (char)'R';
		txBuffer_[j++] = (char)'E';
		txBuffer_[j++] = (char)'L';
		txBuffer_[j++] = state;
		bytesToSend = j;
	}

		/// "sCOMP" - Set Visual Components Configuration
	else if (receivedBytes == 56 && rxBuffer_[0] == (char)'s' &&
			rxBuffer_[1] == (char)'C' && rxBuffer_[2] == (char)'O' && rxBuffer_[3] == (char)'M' && rxBuffer_[4] == (char)'P')
	{
		uint32_t address_ = UINT32_FROM_BYTES(rxBuffer_[5], rxBuffer_[6], rxBuffer_[7], rxBuffer_[8]);
		uint8_t segment = rxBuffer_[9];
		struct HeatingVisualComponentControl *heatingControl;
		heatingControl = NULL;
		bool ok = VisualComponent_GetHeatingVisualComponentControlFromAddress(address_, segment, &heatingControl);
		if (ok)
		{
			uint16_t f;
			uint8_t k = 10;
			heatingControl->mode = rxBuffer_[k++];
			heatingControl->periodsPnPtCount = rxBuffer_[k++];
			heatingControl->periodsSaCount = rxBuffer_[k++];
			heatingControl->periodsSuCount = rxBuffer_[k++];
			heatingControl->periodPnPtFrom[0] = rxBuffer_[k++];
			heatingControl->periodPnPtFrom[1] = rxBuffer_[k++];
			heatingControl->periodPnPtFrom[2] = rxBuffer_[k++];
			heatingControl->periodPnPtFrom[3] = rxBuffer_[k++];
			heatingControl->periodSaFrom[0] = rxBuffer_[k++];
			heatingControl->periodSaFrom[1] = rxBuffer_[k++];
			heatingControl->periodSaFrom[2] = rxBuffer_[k++];
			heatingControl->periodSaFrom[3] = rxBuffer_[k++];
			heatingControl->periodSuFrom[0] = rxBuffer_[k++];
			heatingControl->periodSuFrom[1] = rxBuffer_[k++];
			heatingControl->periodSuFrom[2] = rxBuffer_[k++];
			heatingControl->periodSuFrom[3] = rxBuffer_[k++];
			f = rxBuffer_[k++] << 8;
			f |= rxBuffer_[k++];
			heatingControl->manualTemperature = f / 10.0;
			f = rxBuffer_[k++] << 8;
			f |= rxBuffer_[k++];
			heatingControl->periodPnPtTemperature[0] = f / 10.0;
			f = rxBuffer_[k++] << 8;
			f |= rxBuffer_[k++];
			heatingControl->periodPnPtTemperature[1] = f / 10.0;
			f = rxBuffer_[k++] << 8;
			f |= rxBuffer_[k++];
			heatingControl->periodPnPtTemperature[2] = f / 10.0;
			f = rxBuffer_[k++] << 8;
			f |= rxBuffer_[k++];
			heatingControl->periodPnPtTemperature[3] = f / 10.0;
			f = rxBuffer_[k++] << 8;
			f |= rxBuffer_[k++];
			heatingControl->periodSaTemperature[0] = f / 10.0;
			f = rxBuffer_[k++] << 8;
			f |= rxBuffer_[k++];
			heatingControl->periodSaTemperature[1] = f / 10.0;
			f = rxBuffer_[k++] << 8;
			f |= rxBuffer_[k++];
			heatingControl->periodSaTemperature[2] = f / 10.0;
			f = rxBuffer_[k++] << 8;
			f |= rxBuffer_[k++];
			heatingControl->periodSaTemperature[3] = f / 10.0;
			f = rxBuffer_[k++] << 8;
			f |= rxBuffer_[k++];
			heatingControl->periodSuTemperature[0] = f / 10.0;
			f = rxBuffer_[k++] << 8;
			f |= rxBuffer_[k++];
			heatingControl->periodSuTemperature[1] = f / 10.0;
			f = rxBuffer_[k++] << 8;
			f |= rxBuffer_[k++];
			heatingControl->periodSuTemperature[2] = f / 10.0;
			f = rxBuffer_[k++] << 8;
			f |= rxBuffer_[k++];
			heatingControl->periodSuTemperature[3] = f / 10.0;
			writeVisualControlsTimestamp = GET_MS();
			f = rxBuffer_[k++] << 8;
			f |= rxBuffer_[k++];
			heatingControl->maxFloorTemperature = f / 10.0;
			f = rxBuffer_[k++] << 8;
			f |= rxBuffer_[k++];
			heatingControl->hysteresisTemperature = f / 100.0;
		}

		txBuffer_[j++] = (char)'s';
		txBuffer_[j++] = (char)'C';
		txBuffer_[j++] = (char)'O';
		txBuffer_[j++] = (char)'M';
		txBuffer_[j++] = (char)'P';
		txBuffer_[j++] = ok ? 0 : 1;
		bytesToSend = j;
	}

		/// "gCOMP" - Get Visual Components Configuration
	else if (receivedBytes == 10 && rxBuffer_[0] == (char)'g' &&
			rxBuffer_[1] == (char)'C' && rxBuffer_[2] == (char)'O' && rxBuffer_[3] == (char)'M' && rxBuffer_[4] == (char)'P')
	{
		uint32_t address_ = UINT32_FROM_BYTES(rxBuffer_[5], rxBuffer_[6], rxBuffer_[7], rxBuffer_[8]);
		uint8_t segment = rxBuffer_[9];
		struct HeatingVisualComponentControl *heatingControl;
		heatingControl = NULL;
		bool ok = VisualComponent_GetHeatingVisualComponentControlFromAddress(address_, segment, &heatingControl);

		txBuffer_[j++] = (char)'g';
		txBuffer_[j++] = (char)'C';
		txBuffer_[j++] = (char)'O';
		txBuffer_[j++] = (char)'M';
		txBuffer_[j++] = (char)'P';
		txBuffer_[j++] = ok ? 0 : 1;
		if (ok)
		{
			txBuffer_[j++] = heatingControl->mode;
			txBuffer_[j++] = heatingControl->periodsPnPtCount;
			txBuffer_[j++] = heatingControl->periodsSaCount;
			txBuffer_[j++] = heatingControl->periodsSuCount;
			txBuffer_[j++] = heatingControl->periodPnPtFrom[0];
			txBuffer_[j++] = heatingControl->periodPnPtFrom[1];
			txBuffer_[j++] = heatingControl->periodPnPtFrom[2];
			txBuffer_[j++] = heatingControl->periodPnPtFrom[3];
			txBuffer_[j++] = heatingControl->periodSaFrom[0];
			txBuffer_[j++] = heatingControl->periodSaFrom[1];
			txBuffer_[j++] = heatingControl->periodSaFrom[2];
			txBuffer_[j++] = heatingControl->periodSaFrom[3];
			txBuffer_[j++] = heatingControl->periodSuFrom[0];
			txBuffer_[j++] = heatingControl->periodSuFrom[1];
			txBuffer_[j++] = heatingControl->periodSuFrom[2];
			txBuffer_[j++] = heatingControl->periodSuFrom[3];
			txBuffer_[j++] = UINT32_1BYTE((int)(heatingControl->manualTemperature * 10.0));
			txBuffer_[j++] = UINT32_0BYTE((int)(heatingControl->manualTemperature * 10.0));
			txBuffer_[j++] = UINT32_1BYTE((int)(heatingControl->periodPnPtTemperature[0] * 10.0));
			txBuffer_[j++] = UINT32_0BYTE((int)(heatingControl->periodPnPtTemperature[0] * 10.0));
			txBuffer_[j++] = UINT32_1BYTE((int)(heatingControl->periodPnPtTemperature[1] * 10.0));
			txBuffer_[j++] = UINT32_0BYTE((int)(heatingControl->periodPnPtTemperature[1] * 10.0));
			txBuffer_[j++] = UINT32_1BYTE((int)(heatingControl->periodPnPtTemperature[2] * 10.0));
			txBuffer_[j++] = UINT32_0BYTE((int)(heatingControl->periodPnPtTemperature[2] * 10.0));
			txBuffer_[j++] = UINT32_1BYTE((int)(heatingControl->periodPnPtTemperature[3] * 10.0));
			txBuffer_[j++] = UINT32_0BYTE((int)(heatingControl->periodPnPtTemperature[3] * 10.0));
			txBuffer_[j++] = UINT32_1BYTE((int)(heatingControl->periodSaTemperature[0] * 10.0));
			txBuffer_[j++] = UINT32_0BYTE((int)(heatingControl->periodSaTemperature[0] * 10.0));
			txBuffer_[j++] = UINT32_1BYTE((int)(heatingControl->periodSaTemperature[1] * 10.0));
			txBuffer_[j++] = UINT32_0BYTE((int)(heatingControl->periodSaTemperature[1] * 10.0));
			txBuffer_[j++] = UINT32_1BYTE((int)(heatingControl->periodSaTemperature[2] * 10.0));
			txBuffer_[j++] = UINT32_0BYTE((int)(heatingControl->periodSaTemperature[2] * 10.0));
			txBuffer_[j++] = UINT32_1BYTE((int)(heatingControl->periodSaTemperature[3] * 10.0));
			txBuffer_[j++] = UINT32_0BYTE((int)(heatingControl->periodSaTemperature[3] * 10.0));
			txBuffer_[j++] = UINT32_1BYTE((int)(heatingControl->periodSuTemperature[0] * 10.0));
			txBuffer_[j++] = UINT32_0BYTE((int)(heatingControl->periodSuTemperature[0] * 10.0));
			txBuffer_[j++] = UINT32_1BYTE((int)(heatingControl->periodSuTemperature[1] * 10.0));
			txBuffer_[j++] = UINT32_0BYTE((int)(heatingControl->periodSuTemperature[1] * 10.0));
			txBuffer_[j++] = UINT32_1BYTE((int)(heatingControl->periodSuTemperature[2] * 10.0));
			txBuffer_[j++] = UINT32_0BYTE((int)(heatingControl->periodSuTemperature[2] * 10.0));
			txBuffer_[j++] = UINT32_1BYTE((int)(heatingControl->periodSuTemperature[3] * 10.0));
			txBuffer_[j++] = UINT32_0BYTE((int)(heatingControl->periodSuTemperature[3] * 10.0));
			txBuffer_[j++] = UINT32_1BYTE((int)(heatingControl->maxFloorTemperature * 10.0));
			txBuffer_[j++] = UINT32_0BYTE((int)(heatingControl->maxFloorTemperature * 10.0));
			txBuffer_[j++] = UINT32_1BYTE((int)(heatingControl->hysteresisTemperature * 100.0));
			txBuffer_[j++] = UINT32_0BYTE((int)(heatingControl->hysteresisTemperature * 100.0));
		}
		bytesToSend = j;
	}
#endif

#ifdef ISR_CU
		/// "RTC" - Get RTC date and time
	else if (receivedBytes == 4 && rxBuffer_[0] == (char)'R' && rxBuffer_[1] == (char)'T' &&
			rxBuffer_[2] == (char)'C' && rxBuffer_[3] == 0)
	{
		if (rxBuffer_[3] == 0)
		{
			txBuffer_[j++] = (char)'R';
			txBuffer_[j++] = (char)'T';
			txBuffer_[j++] = (char)'C';
			struct tm timeinfo;
			memset(&timeinfo, 0, sizeof(timeinfo));
			txBuffer_[j++] = RTC_DS3231_Read(&timeinfo);	/// too long (1s) if module not found
			txBuffer_[j++] = timeinfo.tm_year - 2000;
			txBuffer_[j++] = timeinfo.tm_mon;
			txBuffer_[j++] = timeinfo.tm_mday;
			txBuffer_[j++] = timeinfo.tm_wday;
			txBuffer_[j++] = timeinfo.tm_hour;
			txBuffer_[j++] = timeinfo.tm_min;
			txBuffer_[j++] = timeinfo.tm_sec;
			bytesToSend = j;
		}
	}
#endif

	if (bytesToSend == 0)
		return 0;

	return EncodePacket(txBuffer, txBufferLength, communication->packetId, 0x08080808, cfgBootloader.deviceAddress, // $$
			true, bytesToSend);
}
