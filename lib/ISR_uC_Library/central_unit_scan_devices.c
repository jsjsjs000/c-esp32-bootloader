#include "../../include/main.h"
#if !defined(BOOTLOADER) && defined(ISR_CU)
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "esp_log.h"
#include "common.h"
#include "packets.h"
#include "communication.h"
#include "commands.h"
#include "configuration_bootloader.h"
#include "device_item2.h"
#include "device_item.h"
#include "central_unit_scan_devices.h"
#include "rtc.h"

static const char *TAG = "CentralUnitScanDevices";
static const uint8_t LogLevel = 0; /// 0 - 2

static uint32_t lastLog = 0;

bool CentralUnitScanDevices_IsAnyDeviceItem(struct Communication *communication)
{
	for (uint16_t i = 0; i < devicesItems.devicesItemsCount; i++)
		if (devicesItems.devicesItems[i].lineNumber == communication->lineNumber)
			return true;
	return false;
}

void CentralUnitScanDevices_Initialize(struct Communication *communication,
		struct CentralUnitScanDevices *scanDevices)
{
	scanDevices->isAnyDeviceItemToScan = CentralUnitScanDevices_IsAnyDeviceItem(communication);
	scanDevices->isFirstSynchronizationSend = false;
	scanDevices->lastSynchronizationSend = 0;
	scanDevices->startScanCycle = 0;
	scanDevices->deviceItemIndex = 0;
}

float CentralUnitScanDevices_GetSettingTemperatureForTime(struct HeatingVisualComponentControl *heatingComponentControl,
		uint8_t day, uint8_t rtcHM, float initialTemperature)
{
	uint8_t periodsCount;
	uint8_t *periodFrom;
	float *periodTemperature;
	if (day == 0)
	{
		periodsCount = heatingComponentControl->periodsSuCount;
		periodFrom = heatingComponentControl->periodSuFrom;
		periodTemperature = heatingComponentControl->periodSuTemperature;
	}
	else if (day == 6)
	{
		periodsCount = heatingComponentControl->periodsSaCount;
		periodFrom = heatingComponentControl->periodSaFrom;
		periodTemperature = heatingComponentControl->periodSaTemperature;
	}
	else
	{
		periodsCount = heatingComponentControl->periodsPnPtCount;
		periodFrom = heatingComponentControl->periodPnPtFrom;
		periodTemperature = heatingComponentControl->periodPnPtTemperature;
	}

	if (periodsCount == 1)
		return periodTemperature[0];

	float temperature = initialTemperature;
	for (int i = 0; i < periodsCount; i++)
	{
		if (rtcHM < periodFrom[i])
			return temperature;
		temperature = periodTemperature[i];
	}
	return periodTemperature[periodsCount - 1];
}

float CentralUnitScanDevices_GetSettingTemperature(struct HeatingVisualComponentControl *heatingComponentControl)
{
	if (!RTC_isSetRtc)
		return NAN;

	time_t now;
	struct tm timeinfo;
	time(&now);
	localtime_r(&now, &timeinfo);
	uint8_t rtcHM = timeinfo.tm_hour * 10 + timeinfo.tm_min / 10;

	float initialTemperature = CentralUnitScanDevices_GetSettingTemperatureForTime(heatingComponentControl,
			(timeinfo.tm_wday - 1 + 7) % 7, 24 * 10, 0);
	return CentralUnitScanDevices_GetSettingTemperatureForTime(heatingComponentControl,
			timeinfo.tm_wday, rtcHM, initialTemperature);
}

	/// return true if any diffrence detected
bool CentralUnitScanDevices_SendDevicesStates(struct Communication *communication)
{
	if (communication->sendCommand == SEND_COMMAND_NONE && RTC_isSetRtc && GET_MS() >= CU_SYNCHRONIZATION_STARTUP_DELAY2)
	{																															/// wait for startup other devices with bootloader
// 			/// synchronize relays from devicesItems / relayControl
// 		uint16_t i = 0;
// 		struct DeviceItem *deviceItem;
// 		while (i < devicesItems.devicesItemsCount)
// 		{
// 			deviceItem = &devicesItems.devicesItems[i++];
// 			if (deviceItem->lineNumber == communication->lineNumber &&
// 					deviceItem->hardwareType2 == CONFIGURATION_HARDWARE_TYPE2_REL &&
// 					!deviceItem->deviceItemStatus->error)
// 			{
// 				struct RelayStatus *relayStatus;
// 				relayStatus = deviceItem->deviceItemStatus->status;
// 				struct RelayControl *relayControl;
// 				relayControl = deviceItem->deviceItemControl;
// 				for (uint8_t j = 0; j < deviceItem->hardwareSegmentsCount; j++)
// 				{
// 					if (relayControl->relayState != relayStatus->relayState &&
// 							GET_MS() - relayControl->updateTime >= 1000)
// 					{
// // LOGI(TAG, "  different relay state %.8x %d: %d != %d, line %d", deviceItem->address, j, relayControl->relayState, relayStatus->relayState, communication->lineNumber);
// 						relayControl->updateTime = GET_MS();
//
// 						communication->sendCommandBytesToSend = Command_PrepareRelaySetState(
// 								communication->bufferTx, sizeof(communication->bufferTx), j, relayControl->relayState);
// 						communication->sendCommandAddress = deviceItem->address;
// 						communication->sendCommandSegmentsCount = deviceItem->hardwareSegmentsCount;
// 						communication->sendCommand = SEND_COMMAND_SET_RELAY_STATE;
// 						return true;
// 					}
// 					relayStatus++;
// 					relayControl++;
// 				}
// 			}
// 		}

			/// thermostats
		for (uint16_t i = 0; i < heatingDevicesComponentsCount; i++)
		{
			struct HeatingVisualComponent *heatingComponent = &heatingDevicesComponents[i];
			struct DeviceItem *deviceItemRelay;
			deviceItemRelay = NULL;
			if (!DeviceItem_GetDeviceItemFromAddress(heatingComponent->deviceItem, &deviceItemRelay))
				continue;

			if (deviceItemRelay->lineNumber != communication->lineNumber)
				continue;

			bool isInitialized = true;
			bool error = false;
			bool errorLess15s = false;
			float tempFloor = NAN;
			float tempAir = NAN;

			struct DeviceItemStatus *relayDeviceItemStatus;
			relayDeviceItemStatus = deviceItemRelay->deviceItemStatus;
			if (!relayDeviceItemStatus->initialized)
				isInitialized = false;
			if (relayDeviceItemStatus->error)
				continue;

			struct RelayStatus *relayStatus;
			relayStatus = relayDeviceItemStatus->status;
			relayStatus = &relayStatus[heatingComponent->deviceSegment];
			
			struct RelayControl *relayControl;
			relayControl = deviceItemRelay->deviceItemControl;
			relayControl = &relayControl[heatingComponent->deviceSegment];
			
			struct HeatingVisualComponentControl *heatingComponentControl;
			heatingComponentControl = heatingComponent->heatingVisualComponentControl;
			struct HeatingVisualComponentSubItem *subItem = heatingComponent->subComponents;
			for (uint16_t j = 0; j < heatingComponent->subComponentsCount; j++)
			{
				struct DeviceItem *deviceItem;
				deviceItem = NULL;
				if (!DeviceItem_GetDeviceItemFromAddress(subItem->deviceItem, &deviceItem))
				{
					error = true;
					subItem++;
					continue;
				}
				
				if (deviceItem->hardwareType2 != CONFIGURATION_HARDWARE_TYPE2_TEMP)
				{
					subItem++;
					continue;
				}
				
				struct DeviceItemStatus *deviceItemStatus;
				deviceItemStatus = deviceItem->deviceItemStatus;
				if (!deviceItemStatus->initialized)
					isInitialized = false;
				if (deviceItemStatus->error)
				{
					error = true;
					if (GET_MS() - deviceItemStatus->errorFrom <= CU_SYNCHRONIZATION_ERROR_TIMEOUT)
						errorLess15s = true;
				}

				struct TemperatureStatus *temperatureStatus;
				temperatureStatus = deviceItemStatus->status;
				temperatureStatus = &temperatureStatus[subItem->deviceSegment];
				if (temperatureStatus->error)
				{
					error = true;
					if (GET_MS() - temperatureStatus->errorFrom <= CU_SYNCHRONIZATION_ERROR_TIMEOUT)
						errorLess15s = true;
				}

				if (!error)
				{
					if (j == 0)
						tempFloor = temperatureStatus->temperature / 16.0;
					else if (j == 1)
						tempAir = temperatureStatus->temperature / 16.0;
				}

				subItem++;
			}

			bool newRelayState = false;
			if (isInitialized && !error)
			{
				float setTemperature = NAN;
				if (heatingComponentControl->mode == HEATING_VISUAL_COMPONENT_MODE_AUTO && RTC_isSetRtc)
					setTemperature = CentralUnitScanDevices_GetSettingTemperature(heatingComponentControl);
				else if (heatingComponentControl->mode == HEATING_VISUAL_COMPONENT_MODE_MANUAL)
					setTemperature = heatingComponentControl->manualTemperature;

				if (setTemperature != NAN)
				{
					if (heatingComponent->subComponentsCount == 1)	/// one temperature
					{
						if (relayStatus->relayState)								/// hysteresis
							newRelayState = tempFloor < setTemperature + heatingComponentControl->hysteresisTemperature;
						else
							newRelayState = tempFloor < setTemperature - heatingComponentControl->hysteresisTemperature;

						if (newRelayState)													/// max floor temperature with hysteresis
						{
							if (relayStatus->relayState && tempFloor > heatingComponentControl->maxFloorTemperature +
									heatingComponentControl->hysteresisTemperature)
								newRelayState = false;
							else if (!relayStatus->relayState && tempFloor > heatingComponentControl->maxFloorTemperature -
									heatingComponentControl->hysteresisTemperature)
								newRelayState = false;
						}
					}
					else																						/// two temperatures
					{
						if (relayStatus->relayState)								/// hysteresis
							newRelayState = tempAir < setTemperature + heatingComponentControl->hysteresisTemperature;
						else
							newRelayState = tempAir < setTemperature - heatingComponentControl->hysteresisTemperature;

						if (newRelayState)													/// max floor temperature with hysteresis
						{
							if (relayStatus->relayState && tempFloor > heatingComponentControl->maxFloorTemperature +
									heatingComponentControl->hysteresisTemperature)
								newRelayState = false;
							else if (!relayStatus->relayState && tempFloor > heatingComponentControl->maxFloorTemperature -
									heatingComponentControl->hysteresisTemperature)
								newRelayState = false;
						}
					}
				}
			}
			relayControl->relayState = newRelayState;

			if (LogLevel >= 2 && GET_MS() >= lastLog + 2000)
			{
				if (heatingComponent->subComponentsCount == 1)
					LOGI(TAG, "Heating component %d: temp: %.1f C, error: %d", i, tempFloor, error);
				else
					LOGI(TAG, "Heating component %d: floor: %.1f C, air: %.1f C, error: %d", i, tempFloor, tempAir, error);
				if (heatingComponentControl->mode == HEATING_VISUAL_COMPONENT_MODE_OFF)
					LOGI(TAG, "  mode: off");
				else if (heatingComponentControl->mode == HEATING_VISUAL_COMPONENT_MODE_AUTO)
					LOGI(TAG, "  mode: auto");
				else if (heatingComponentControl->mode == HEATING_VISUAL_COMPONENT_MODE_MANUAL)
					LOGI(TAG, "  mode: manual, temp: %.1f C", heatingComponentControl->manualTemperature);
				LOGI(TAG, "  relayState: %d, newState: %d", relayStatus->relayState, relayControl->relayState);
			}

			if (relayControl->relayState != relayStatus->relayState && GET_MS() - relayControl->updateTime >= 1000 && !errorLess15s)
			{
				if (LogLevel >= 1)
				{
					if (heatingComponent->subComponentsCount == 1)
						LOGI(TAG, "Heating component %d: floor: %.1f C, error: %d", i, tempFloor, error);
					else
						LOGI(TAG, "Heating component %d: floor: %.1f C, air: %.1f C, error: %d", i, tempFloor, tempAir, error);
					if (heatingComponentControl->mode == HEATING_VISUAL_COMPONENT_MODE_OFF)
						LOGI(TAG, "  mode: off");
					else if (heatingComponentControl->mode == HEATING_VISUAL_COMPONENT_MODE_AUTO)
						LOGI(TAG, "  mode: auto");
					else if (heatingComponentControl->mode == HEATING_VISUAL_COMPONENT_MODE_MANUAL)
						LOGI(TAG, "  mode: manual, temp: %.1f C", heatingComponentControl->manualTemperature);
					LOGI(TAG, "  relayState: %d, newState: %d", relayStatus->relayState, relayControl->relayState);
					LOGI(TAG, "    different relay state %.8x %d: %d != %d, line %d", deviceItemRelay->address, heatingComponent->deviceSegment,
							relayControl->relayState, relayStatus->relayState, communication->lineNumber);
				}

				relayControl->updateTime = GET_MS();

				communication->sendCommandBytesToSend = Command_PrepareRelaySetState(
						communication->bufferTx, sizeof(communication->bufferTx),
						heatingComponent->deviceSegment, relayControl->relayState);
				communication->sendCommandAddress = deviceItemRelay->address;
				communication->sendCommandSegmentsCount = deviceItemRelay->hardwareSegmentsCount;
				communication->sendCommand = SEND_COMMAND_SET_RELAY_STATE;
				return true;
			}
		}
	}

	if (LogLevel >= 2 && communication->lineNumber == LINE_UART2 && GET_MS() >= lastLog + 2000)
	{
		LOGI(TAG, "");
		lastLog = GET_MS();
	}

	return false;
}

void CentralUnitScanDevices_Receive(struct Communication *communication)
{
	bool isAnswer;
	uint16_t bytesReceivedCorrect = CheckFrameInBuffer(communication, communication->bufferRx, &communication->bufferRxIndex,
			communication->lastSendCommandPacketId, communication->lastSendCommandAddress, &isAnswer);
	if (bytesReceivedCorrect > 0)
	{
		communication->lastReceived = GET_MS();
		if (isAnswer)
		{
// LOGI(TAG, "Received bytes correct %d: %02x %02x %02x", bytesReceivedCorrect, communication->bufferRx[15+0], communication->bufferRx[15+1], communication->bufferRx[15+2]);
			Command_DecodeAnswer(communication, bytesReceivedCorrect, communication->bufferRx);
		}
		communication->sendCommand = SEND_COMMAND_NONE;
	}
}

void CentralUnitScanDevices_GetNextDeviceItem(struct Communication *communication,
		struct DeviceItem **deviceItem, bool *isEndLoopAndWait, struct CentralUnitScanDevices *scanDevices)
{
	if (scanDevices->deviceItemIndex == 0)
	{
		if (GET_MS() - scanDevices->startScanCycle <= 1000)
		{
			*isEndLoopAndWait = true;
			return;
		}
		else
			scanDevices->startScanCycle = GET_MS();
	}

	bool ok = false;
	struct DeviceItem *deviceItem_;
	do
	{
			/// ** - https://stackoverflow.com/questions/35324934/c-parameter-set-but-not-used
		*deviceItem = &devicesItems.devicesItems[scanDevices->deviceItemIndex++];
		deviceItem_ = *deviceItem;
		ok = deviceItem_->lineNumber == communication->lineNumber;
// LOGI(TAG, "-line %d, index %d, device line %d, ok %d", communication->lineNumber, deviceItemIndex - 1, deviceItem_->lineNumber, ok);
	}
	while (!ok && scanDevices->deviceItemIndex < devicesItems.devicesItemsCount);
	*isEndLoopAndWait = !ok;
// LOGI(TAG, "+line %d, index %d, device line %d, ok %d", communication->lineNumber, scanDevices->deviceItemIndex - 1, deviceItem_->lineNumber, ok);

	if (scanDevices->deviceItemIndex >= devicesItems.devicesItemsCount)
		scanDevices->deviceItemIndex = 0;
}

bool CentralUnitScanDevices_Loop(struct Communication *communication,
		struct CentralUnitScanDevices *scanDevices)
{
	if (communication->sendCommand == SEND_COMMAND_NONE && GET_MS() - scanDevices->lastSynchronizationSend >= 500 &&
			GET_MS() >= CU_SYNCHRONIZATION_STARTUP_DELAY1 &&																				/// wait for startup other devices with bootloader
			((!scanDevices->isFirstSynchronizationSend && GET_MS() % 1000 <= 10) ||
				(scanDevices->isFirstSynchronizationSend && GET_MS() % CU_SYNCHRONIZATION_TIME <= 20)))
	{
		scanDevices->isFirstSynchronizationSend = true;
		scanDevices->lastSynchronizationSend = GET_MS();
		uint16_t bytesToSend = Command_PrepareSynchronization(communication->bufferTx, sizeof(communication->bufferTx));
// LOGI(TAG, "Send Synchronization");

		communication->sendCommandBytesToSend = bytesToSend;
		communication->sendCommandAddress = BROADCAST;
		communication->sendCommand = SEND_COMMAND_NONE;
		return true;
	}

	if (scanDevices->isAnyDeviceItemToScan && scanDevices->isFirstSynchronizationSend)
	{
		if (communication->sendCommand == SEND_COMMAND_NONE)
		{
			struct DeviceItem *deviceItem;
			deviceItem = NULL;
			bool isEndLoopAndWait;
			CentralUnitScanDevices_GetNextDeviceItem(communication, &deviceItem, &isEndLoopAndWait, scanDevices);
			if (!isEndLoopAndWait)
			{
				uint16_t bytesToSend = 0;
				uint8_t command = SEND_COMMAND_NONE;
				if (deviceItem->hardwareType2 == CONFIGURATION_HARDWARE_TYPE2_TEMP)
				{
					bytesToSend = Command_PrepareTemperature(communication->bufferTx, sizeof(communication->bufferTx));
					command = SEND_COMMAND_GET_TEMPERATURE;
				}
				else if (deviceItem->hardwareType2 == CONFIGURATION_HARDWARE_TYPE2_REL)
				{
					bytesToSend = Command_PrepareRelaysStatus(communication->bufferTx, sizeof(communication->bufferTx));
					command = SEND_COMMAND_GET_RELAY_STATE;
				}

				if (bytesToSend > 0)
				{
// LOGI(TAG, "send %d bytes to 0x%.8x", bytesToSend, deviceItem->address);

					communication->sendCommandBytesToSend = bytesToSend;
					communication->sendCommandAddress = deviceItem->address;
					communication->sendCommandSegmentsCount = deviceItem->hardwareSegmentsCount;
					communication->sendCommand = command;
					return true;
				}
			}
		}
	}

		/// Send command timeout
	if (communication->sendCommand != SEND_COMMAND_NONE &&
			GET_MS() - communication->lastSendCommandTime >= SEND_COMMAND_TIMEOUT)
	{
		if (communication->sendCommand == SEND_COMMAND_GET_TEMPERATURE)
			Command_AnswerGetTemperatures(communication->sendCommandAddress, false, 0xffffffff, 0, NULL,
					communication->sendCommandSegmentsCount);
		else if (communication->sendCommand == SEND_COMMAND_GET_RELAY_STATE)
			Command_AnswerGetRelaysStatus(communication->sendCommandAddress, false, 0xffffffff, 0, NULL,
					communication->sendCommandSegmentsCount);
		else if (communication->sendCommand == SEND_COMMAND_SET_RELAY_STATE)
			Command_AnswerSetRelaysStatus(communication->sendCommandAddress, false);

		communication->sendCommand = SEND_COMMAND_NONE;
	}

	return false;
}
#endif

/*
	C# Get current temperature for auto mode:
int[] periodFrom = { 6, 13, 15, 22 };
float[] periodTemperature = { 1, 2, 3, 4 };
int periodsCount = 4;

float G(int h, float initialTemperature)
{
	if (periodsCount == 1)
		return periodTemperature[0];

	float temperature = initialTemperature;
	for (int i = 0; i < periodsCount; i++)
	{
		if (h < periodFrom[i])
			return temperature;
		temperature = periodTemperature[i];
	}
	return periodTemperature[periodsCount - 1];
}

float initialTemperature = periodTemperature[periodsCount - 1];

Console.WriteLine($"temp h 5 = {G(5, initialTemperature)} 4");
Console.WriteLine($"temp h 6 = {G(6, initialTemperature)} 1");
Console.WriteLine($"temp h 7 = {G(7, initialTemperature)} 1");
Console.WriteLine($"temp h 13 = {G(13, initialTemperature)} 2");
Console.WriteLine($"temp h 14 = {G(14, initialTemperature)} 2");
Console.WriteLine($"temp h 15 = {G(15, initialTemperature)} 3");
Console.WriteLine($"temp h 20 = {G(20, initialTemperature)} 3");
Console.WriteLine($"temp h 22 = {G(22, initialTemperature)} 4");
Console.WriteLine($"temp h 23 = {G(23, initialTemperature)} 4");
Console.ReadKey();
*/
