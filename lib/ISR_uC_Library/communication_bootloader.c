#include "../../include/main.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#ifdef ESP32
	#include "freertos/FreeRTOS.h"
	#include "freertos/task.h"
	#include "esp_system.h"
	#include "esp_task_wdt.h"
	#include "esp_log.h"
	#include "esp_err.h"
#endif
#include "common.h"
#include "crc32.h"
#include "configuration.h"
#include "configuration_bootloader.h"
#include "configuration_program.h"
#include "configuration_boot_state.h"
#ifdef ESP32
	#include "../ISR_ESP32_Library/flash.h"
#endif
#if defined(STM32F0) || defined(STM32F4)
	#include "../../include/stm32f0xx_init.h"
	#include "../ISR_STM32_Library/flash.h"
	#include "../ISR_STM32_Library/rnd.h"
#endif
#include "packets.h"
#include "communication_bootloader.h"
#include "communication.h"

/// programming new program
static uint16_t numberOfPackets = 0;
static uint8_t programmingYear = 0;
static uint8_t programmingMonth = 0;
static uint8_t programmingDay = 0;
static uint8_t programmingHour = 0;
static uint8_t programmingMinute = 0;
static uint8_t packetsResolved[PROGRAMMING_MAX_PACKETS];
static uint16_t lastPacketSize = 0;

uint16_t ExecuteCommunicationCommand_Bootloader(struct Communication* communication,
		uint8_t* txBuffer, uint16_t txBufferLength)
{
	uint16_t bytesToSend = 0;
	uint8_t* txBuffer_ = (uint8_t*)(txBuffer + PACKET_PRE_BYTES);
	uint16_t j = 0;

		/// 0xf1 - Erase Device Memory
	if (communication->command == COMMUNICATION_COMMAND_CLEAR_FLASH)
	{
		communication->command = COMMUNICATION_COMMAND_NONE;
		communication->commandStatus = 2;
		StatusType status = STATUS_OK;
		uint32_t address = FLASH_PROGRAM_ADDRESS;

#if defined(STM32F0) || defined(STM32F4)
		DELAY_MS(10);					/// wait for send response
#endif

		for (uint16_t i = 0; i < communication->commandPagesToClear && status == STATUS_OK; i++)
		{
			status = EraseFlashRegion(address, FLASH_PROGRAM_PAGE_SIZE);
			address += FLASH_PROGRAM_PAGE_SIZE;

#if defined(WATCHDOG) && defined(ESP32)
			if (xTaskGetTickCount() - lastWatchdogReset >= WATCHDOG_RESET_TIME)
			{
				esp_task_wdt_reset();
				lastWatchdogReset = xTaskGetTickCount();
			}
#endif
#if defined(WATCHDOG) && (defined(STM32F0) || defined(STM32F4))
			if (HAL_IWDG_Refresh(&hiwdg) != HAL_OK)
				Error_Handler();
#endif
		}
		communication->commandStatus = status;
	}

		/// 0xf5 - End Device Programming
	else if (communication->command == COMMUNICATION_COMMAND_END_DEVICE_PROGRAMMING)
	{
		communication->command = COMMUNICATION_COMMAND_NONE;

		uint32_t countedCrc = 0;
		StatusType status = STATUS_OK;
		uint32_t adr = FLASH_PROGRAM_ADDRESS;
		for (uint16_t i = 0; i < numberOfPackets - 1 && status == STATUS_OK; i++)
		{
			status = ReadFromFlash(adr, PROGRAMMING_PACKET_SIZE, communication->flashBuffer);
			adr += PROGRAMMING_PACKET_SIZE;
			countedCrc = CalculateCrc32(countedCrc, communication->flashBuffer, 0, PROGRAMMING_PACKET_SIZE);
		}
		if (status == STATUS_OK)
		{
			status = ReadFromFlash(adr, lastPacketSize, communication->flashBuffer);
			countedCrc = CalculateCrc32(countedCrc, communication->flashBuffer, 0, lastPacketSize);
		}

		bool ok = communication->commandProgramCrc32 == countedCrc && status == STATUS_OK;
		if (ok)
		{
#if defined(STM32F0) || defined(STM32F4)
			cfgBootloader.bootState = CONFIGURATION_BOOT_STATE_PROGRAM_EXISTS;
#endif
			cfgBootloader.programedProgramYear = programmingYear;
			cfgBootloader.programedProgramMonth = programmingMonth;
			cfgBootloader.programedProgramDay = programmingDay;
			cfgBootloader.programedProgramHour = programmingHour;
			cfgBootloader.programedProgramMinute = programmingMinute;
		}
		if (WriteConfigurationToFlash(&flashConfigurationBootloader) != STATUS_OK)
			ok = false;

#ifdef ESP32
		if (!SetBootState(CONFIGURATION_BOOT_STATE_PROGRAM_EXISTS))
			ok = false;
#endif

#if defined(STM32F0) || defined(STM32F4)
		if (ok)
			communication->command = COMMUNICATION_COMMAND_JUMP_TO_PROGRAM;
#endif
#ifdef ESP32
		if (ok)
			communication->command = COMMUNICATION_COMMAND_RESET;
#endif

		txBuffer_[j++] = 0xf5;
		txBuffer_[j++] = ok ? 0 : 1;
		bytesToSend = j;
	}

		/// 0xf9 - Set Device Address
	else if (communication->command == COMMUNICATION_COMMAND_SET_DEVICE_ADDRESS)
	{
		communication->command = COMMUNICATION_COMMAND_NONE;

		cfgBootloader.hardwareType1 = communication->commandSetHardwareType1;
		cfgBootloader.hardwareType2 = communication->commandSetHardwareType2;
		cfgBootloader.hardwareSegmentsCount = communication->commandSetHardwareSegmentsCount;
		cfgBootloader.hardwareVersion = communication->commandSetHardwareVersion;
		cfgBootloader.deviceAddress = communication->commandSetAddress;
		cfgBootloader.flashEncryptionKey = Random32();
		bool ok = WriteConfigurationToFlash(&flashConfigurationBootloader) == STATUS_OK;

		txBuffer_[j++] = 0xf9;
		txBuffer_[j++] = ok ? 0 : 1;
		bytesToSend = j;
	}

		/// Jump to program
	else if (communication->command == COMMUNICATION_COMMAND_JUMP_TO_PROGRAM)
	{
		communication->command = COMMUNICATION_COMMAND_NONE;
#if defined(BOOTLOADER) && (defined(STM32F0) || defined(STM32F4))
		Main_JumpToProgram();
#endif
	}

	if (bytesToSend == 0)
		return 0;

	return EncodePacket(txBuffer, txBufferLength, communication->packetId, 0x08080808, cfgBootloader.deviceAddress,
			true, bytesToSend);
}

void ResetResolvedPackets(void)
{
	for (uint16_t i = 0; i < sizeof(packetsResolved); i++)
		packetsResolved[i] = 0;
}

uint16_t ReceiveForRequest_Bootloader(struct Communication* communication,
		uint16_t receivedBytes, uint8_t* rxBuffer, uint8_t* txBuffer, uint16_t txBufferLength)
{
	uint16_t bytesToSend = 0;
	uint8_t* rxBuffer_ = (uint8_t*)(rxBuffer + PACKET_PRE_BYTES);
	uint8_t* txBuffer_ = (uint8_t*)(txBuffer + PACKET_PRE_BYTES);
	uint32_t address = UINT32_FROM_ARRAY_BE(rxBuffer, 11);
	uint16_t j = 0;

		/// 0xf1 - Erase Device Memory
	if (receivedBytes == 8 && rxBuffer_[0] == 0xf1 && address != BROADCAST)
	{
		numberOfPackets = (rxBuffer_[1] << 8) | rxBuffer_[2];
		programmingYear = rxBuffer_[3];
		programmingMonth = rxBuffer_[4];
		programmingDay = rxBuffer_[5];
		programmingHour = rxBuffer_[6];
		programmingMinute = rxBuffer_[7];
		uint32_t delay = 0;
#if defined(STM32F0) || defined(STM32F4)
		delay += 10;					/// wait for send response
#endif

		if (numberOfPackets > 0 && numberOfPackets < PROGRAMMING_MAX_PACKETS)
		{
			communication->commandPagesToClear =
					(uint32_t)numberOfPackets * PROGRAMMING_PACKET_SIZE / FLASH_PROGRAM_PAGE_SIZE;
			if ((uint32_t)numberOfPackets * PROGRAMMING_PACKET_SIZE % FLASH_PROGRAM_PAGE_SIZE > 0)
				communication->commandPagesToClear++;
			communication->command = COMMUNICATION_COMMAND_CLEAR_FLASH;

			delay = (uint32_t)communication->commandPagesToClear * FLASH_ERASE_PAGE_TIME_TO_COUNT / 50;
			if (communication->commandPagesToClear * FLASH_ERASE_PAGE_TIME_TO_COUNT % 50 > 0)
				delay++;
		}
		ResetResolvedPackets();

		txBuffer_[j++] = 0xf1;
		txBuffer_[j++] = (numberOfPackets > 0 && numberOfPackets < PROGRAMMING_MAX_PACKETS) ? 0 : 1;
		txBuffer_[j++] = (uint8_t)(delay >> 8);    /// how many milliseconds wait for clear program / 50
		txBuffer_[j++] = (uint8_t)(delay & 0xff);
		bytesToSend = j;
	}

		/// 0xf2 - Check Is Clear Device Memory
	else if (receivedBytes == 1 && rxBuffer_[0] == 0xf2 && address != BROADCAST)
	{
		txBuffer_[j++] = 0xf2;
		txBuffer_[j++] = communication->commandStatus != STATUS_OK;
		bytesToSend = j;
	}

		/// 0xf3 - Send Packet To Device
	else if (receivedBytes <= PROGRAMMING_PACKET_SIZE + 5 && rxBuffer_[0] == 0xf3)
	{
		uint16_t packetId_ = (rxBuffer_[1] << 8) | rxBuffer_[2];
		uint16_t packetLength = MIN(PROGRAMMING_PACKET_SIZE, (rxBuffer_[3] << 8) | rxBuffer_[4]);

		if (packetsResolved[packetId_] == 0 && numberOfPackets < PROGRAMMING_MAX_PACKETS)
		{
			uint8_t *rxBuffer__ = rxBuffer_ + 1 + 2 + 2;
			uint32_t address = FLASH_PROGRAM_ADDRESS + packetId_ * PROGRAMMING_PACKET_SIZE;
			if (WriteToFlash(address, packetLength, rxBuffer__) == STATUS_OK)
				packetsResolved[packetId_] = 1;
			else
				packetsResolved[packetId_] = 0;

			if (packetId_ == numberOfPackets - 1)
				lastPacketSize = packetLength;
		}
	}

		/// 0xf4 - Get Not Resolved Packets List In Device - first 20
	else if (receivedBytes == 1 && rxBuffer_[0] == 0xf4 && address != BROADCAST)
	{
		uint16_t packetsCount = 0;
		uint16_t k = 2;
		for (int16_t i = 0; i < PROGRAMMING_MAX_PACKETS && i < numberOfPackets &&
				packetsCount <= PROGRAMMING_SEND_MAX_NOT_RESOLVED_PACKETS; i++)
			if (packetsResolved[i] == 0)
			{
				txBuffer_[k++] = (uint8_t)(i >> 8);
				txBuffer_[k++] = (uint8_t)(i & 0xff);
				packetsCount++;
			}

		txBuffer_[j++] = 0xf4;
		txBuffer_[j++] = packetsCount;

		bytesToSend = j + packetsCount * 2;
	}

		/// 0xf5 - End Device Programming
	else if (receivedBytes == 5 && rxBuffer_[0] == 0xf5 && address != BROADCAST)
	{
		communication->commandProgramCrc32 = UINT32_FROM_ARRAY_BE(rxBuffer_, 1);
		if (numberOfPackets > 0)
			communication->command = COMMUNICATION_COMMAND_END_DEVICE_PROGRAMMING;
		else
		{
			txBuffer_[j++] = 0xf5;
			txBuffer_[j++] = 1;
			bytesToSend = j;
		}
	}

		/// 0xf9 - Set Device Address
	else if (receivedBytes == 9 && rxBuffer_[0] == 0xf9 && address != BROADCAST)
	{
		communication->commandSetHardwareType1 = rxBuffer_[1];
		communication->commandSetHardwareType2 = rxBuffer_[2];
		communication->commandSetHardwareSegmentsCount = rxBuffer_[3];
		communication->commandSetHardwareVersion = rxBuffer_[4];
		communication->commandSetAddress = UINT32_FROM_ARRAY_BE(rxBuffer_, 5);
		if (communication->commandSetAddress == UINT32_MAX)
			communication->commandSetAddress = Random32();
		communication->command = COMMUNICATION_COMMAND_SET_DEVICE_ADDRESS;
	}

	if (bytesToSend == 0)
		return 0;

	return EncodePacket(txBuffer, txBufferLength, communication->packetId, 0x08080808, cfgBootloader.deviceAddress,
			true, bytesToSend);
}
