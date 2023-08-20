#include "../../include/main.h"
#if !defined(BOOTLOADER) && defined(ISR_CU)
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "common.h"
#include "device_item2.h"
#include "device_item.h"
#include "configuration_bootloader.h"
#include "configuration_cu_control.h"
#include "communication.h"
#include "central_unit_scan_devices.h"
#include "rtc.h"
#include "../ISR_ESP32_Library/adc.h"

static const char *TAG = "DeviceItem";
static const uint8_t LogLevel = 0; /// 0 - 1

struct DeviceItem devicesItems;
struct DeviceItemStatus *devicesItemsStatus;

void DeviceItem_Initialize(void)
{
#ifdef CONFIGURATION_TEST
	const uint16_t devicesItemsCount = 3;		/// test devices
#endif
#if defined(CONFIGURATION_ADAM_KUKUC) || defined(CONFIGURATION_ADAM_KUKUC_TEST)
	const uint16_t devicesItemsCount = 15;	/// Adam Kukuc configuration
#endif

	struct DeviceItem *deviceItem;
	deviceItem = (struct DeviceItem*)malloc(devicesItemsCount * sizeof(struct DeviceItem));
	devicesItemsStatus = (struct DeviceItemStatus*)malloc(devicesItemsCount * sizeof(struct DeviceItemStatus));
	uint16_t i = 0;

	devicesItems.lineNumber = LINE_NONE;
	devicesItems.address = 0;
	devicesItems.hardwareType1 = CONFIGURATION_HARDWARE_TYPE1_DIN;
	devicesItems.hardwareType2 = CONFIGURATION_HARDWARE_TYPE2_CU;
	devicesItems.hardwareSegmentsCount = 2;
	devicesItems.devicesItems = deviceItem;
	devicesItems.devicesItemsCount = devicesItemsCount;

#ifdef CONFIGURATION_TEST
		/// test devices addresses: DIN-CU f938c9cd, DIN-TEMP-4 66a8c8e8, DIN-REL-2 e423a30f, BOX-TEMP-2 262f6efd
	DeviceItem_AddItem(deviceItem, i++, 0x66a8c8e8, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_DIN, CONFIGURATION_HARDWARE_TYPE2_TEMP, 4, 1, 0);
	DeviceItem_AddItem(deviceItem, i++, 0xe423a30f, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_DIN, CONFIGURATION_HARDWARE_TYPE2_REL,  2, 1, 0);
	DeviceItem_AddItem(deviceItem, i++, 0x262f6efd, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);
#endif
#ifdef CONFIGURATION_ADAM_KUKUC
		/// Adam Kukuc configuration: DIN-CU f938c9cd 70:b8:f6:87:62:e3
	DeviceItem_AddItem(deviceItem, i++, 0x74024593, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 7	- Salon i kuchnia - parter
	DeviceItem_AddItem(deviceItem, i++, 0xedd60678, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 10	- Korytarz, wejście, klatka schodowa - parter
	DeviceItem_AddItem(deviceItem, i++, 0xda906314, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 9	- WC - parter
	DeviceItem_AddItem(deviceItem, i++, 0x6efa00b0, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 4	- Sypialnia - I piętro
	DeviceItem_AddItem(deviceItem, i++, 0x66a8c8e8, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 5	- Łazienka - I piętro
	DeviceItem_AddItem(deviceItem, i++, 0x262f6efd, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 6	- Korytarz, klatka schodowa - I piętro
	DeviceItem_AddItem(deviceItem, i++, 0x1b28c309, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 1	- Poddasze
	DeviceItem_AddItem(deviceItem, i++, 0x86246f30, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 17	- Zewnętrzny
	DeviceItem_AddItem(deviceItem, i++, 0xd054c0f1, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 12 - CWU
	DeviceItem_AddItem(deviceItem, i++, 0x6e9dd4c0, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_DIN, CONFIGURATION_HARDWARE_TYPE2_TEMP, 4, 1, 0);	// DIN- 1: Piwnica

	DeviceItem_AddItem(deviceItem, i++, 0xbd2fa348, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_DIN, CONFIGURATION_HARDWARE_TYPE2_REL,  2, 1, 0);	// 1.1 - Salon - parter
																																																																							// 1.2 - Korytarz - parter
	DeviceItem_AddItem(deviceItem, i++, 0xbc9ebdef, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_DIN, CONFIGURATION_HARDWARE_TYPE2_REL,  2, 1, 0);	// 2.1 - WC - parter
																																																																							// 2.2 - Sypialnia - I piętro
	DeviceItem_AddItem(deviceItem, i++, 0xe423a30f, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_DIN, CONFIGURATION_HARDWARE_TYPE2_REL,  2, 1, 0);	// 3.1 - Łazienka - I piętro
																																																																							// 3.2 - Korytarz - I piętro
	DeviceItem_AddItem(deviceItem, i++, 0xf12e11f2, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_DIN, CONFIGURATION_HARDWARE_TYPE2_REL,  2, 1, 0);	// 4.1 - Poddasze
																																																																							// 4.2 - <brak>
	DeviceItem_AddItem(deviceItem, i++, 0x7bf90ca3, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_DIN, CONFIGURATION_HARDWARE_TYPE2_REL,  2, 1, 0);	// 9.1 - CWU Boiler
																																																																							// 9.2 - CWU Instalacja

	// DeviceItem_AddItem(deviceItem, i++, 0xbd2fa348, LINE_UART1, CONFIGURATION_HARDWARE_TYPE1_DIN, CONFIGURATION_HARDWARE_TYPE2_REL,  2, 1, 0);	// 1
	// DeviceItem_AddItem(deviceItem, i++, 0xbc9ebdef, LINE_UART1, CONFIGURATION_HARDWARE_TYPE1_DIN, CONFIGURATION_HARDWARE_TYPE2_REL,  2, 1, 0);	// 2
	// DeviceItem_AddItem(deviceItem, i++, 0xe423a30f, LINE_UART1, CONFIGURATION_HARDWARE_TYPE1_DIN, CONFIGURATION_HARDWARE_TYPE2_REL,  2, 1, 0);	// 3
	// DeviceItem_AddItem(deviceItem, i++, 0xf12e11f2, LINE_UART1, CONFIGURATION_HARDWARE_TYPE1_DIN, CONFIGURATION_HARDWARE_TYPE2_REL,  2, 1, 0);	// 4
	// DeviceItem_AddItem(deviceItem, i++, 0x7e42b124, LINE_UART1, CONFIGURATION_HARDWARE_TYPE1_DIN, CONFIGURATION_HARDWARE_TYPE2_REL,  2, 1, 0);	// 5
	// DeviceItem_AddItem(deviceItem, i++, 0xe42ab667, LINE_UART1, CONFIGURATION_HARDWARE_TYPE1_DIN, CONFIGURATION_HARDWARE_TYPE2_REL,  2, 1, 0);	// 6
	// DeviceItem_AddItem(deviceItem, i++, 0x33f7971b, LINE_UART1, CONFIGURATION_HARDWARE_TYPE1_DIN, CONFIGURATION_HARDWARE_TYPE2_REL,  2, 1, 0);	// 7
	// DeviceItem_AddItem(deviceItem, i++, 0x89ae4267, LINE_UART1, CONFIGURATION_HARDWARE_TYPE1_DIN, CONFIGURATION_HARDWARE_TYPE2_REL,  2, 1, 0);	// 8
	// DeviceItem_AddItem(deviceItem, i++, 0x7bf90ca3, LINE_UART1, CONFIGURATION_HARDWARE_TYPE1_DIN, CONFIGURATION_HARDWARE_TYPE2_REL,  2, 1, 0);	// 9
	// DeviceItem_AddItem(deviceItem, i++, 0x6e9dd4c0, LINE_UART1, CONFIGURATION_HARDWARE_TYPE1_DIN, CONFIGURATION_HARDWARE_TYPE2_TEMP, 4, 1, 0);
	// DeviceItem_AddItem(deviceItem, i++, 0x1b28c309, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 1
	// DeviceItem_AddItem(deviceItem, i++, 0x83e3ceb7, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 2
	// DeviceItem_AddItem(deviceItem, i++, 0x2543d7e3, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 3
	// DeviceItem_AddItem(deviceItem, i++, 0x6efa00b0, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 4
	// DeviceItem_AddItem(deviceItem, i++, 0x66a8c8e8, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 5
	// DeviceItem_AddItem(deviceItem, i++, 0x262f6efd, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 6
	// DeviceItem_AddItem(deviceItem, i++, 0x74024593, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 7
	// DeviceItem_AddItem(deviceItem, i++, 0xdcdb5dd9, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 8
	// DeviceItem_AddItem(deviceItem, i++, 0xda906314, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 9
	// DeviceItem_AddItem(deviceItem, i++, 0xedd60678, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 10
	// DeviceItem_AddItem(deviceItem, i++, 0x42473737, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 11
	// DeviceItem_AddItem(deviceItem, i++, 0xd054c0f1, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 12
	// DeviceItem_AddItem(deviceItem, i++, 0x00e75578, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 13
	// DeviceItem_AddItem(deviceItem, i++, 0x028322a6, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 14
	// DeviceItem_AddItem(deviceItem, i++, 0x17511b75, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 15
	// DeviceItem_AddItem(deviceItem, i++, 0xfa007455, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 16
	// DeviceItem_AddItem(deviceItem, i++, 0x86246f30, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 17
#endif
#ifdef CONFIGURATION_ADAM_KUKUC_TEST
		/// Adam Kukuc test configuration: DIN-CU 1, DIN-TEMP-4 2, DIN-REL-2 3, BOX-TEMP-2 4
		/// DIN-CU f938c9cd (UART2), DIN-TEMP-4 66a8c8e8, DIN-REL-2 e423a30f, BOX-TEMP-2 262f6efd
	DeviceItem_AddItem(deviceItem, i++, 0x74024593, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 7	- Salon i kuchnia - parter
	DeviceItem_AddItem(deviceItem, i++, 0xedd60678, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 10	- Korytarz, wejście, klatka schodowa - parter
	DeviceItem_AddItem(deviceItem, i++, 0xda906314, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 9	- WC - parter
	DeviceItem_AddItem(deviceItem, i++, 0x6efa00b0, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 4	- Sypialnia - I piętro
	DeviceItem_AddItem(deviceItem, i++, 0x66a8c8e8, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 5	- Łazienka - I piętro
	// DeviceItem_AddItem(deviceItem, i++, 0x262f6efd, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 6	- Korytarz, klatka schodowa - I piętro
	DeviceItem_AddItem(deviceItem, i++, 0x1b28c309, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 1	- Poddasze
	DeviceItem_AddItem(deviceItem, i++, 0x86246f30, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 17	- Zewnętrzny
	DeviceItem_AddItem(deviceItem, i++, 0x262f6efd, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_BOX, CONFIGURATION_HARDWARE_TYPE2_TEMP, 2, 1, 0);	// 8  - CWU
	DeviceItem_AddItem(deviceItem, i++, 0x6e9dd4c0, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_DIN, CONFIGURATION_HARDWARE_TYPE2_TEMP, 4, 1, 0);	// DIN- 1: Piwnica

	DeviceItem_AddItem(deviceItem, i++, 0xbd2fa348, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_DIN, CONFIGURATION_HARDWARE_TYPE2_REL,  2, 1, 0);	// 1.1 - Salon - parter
																																																																							// 1.2 - Korytarz - parter
	DeviceItem_AddItem(deviceItem, i++, 0xbc9ebdef, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_DIN, CONFIGURATION_HARDWARE_TYPE2_REL,  2, 1, 0);	// 2.1 - WC - parter
																																																																							// 2.2 - Sypialnia - I piętro
	// DeviceItem_AddItem(deviceItem, i++, 0xe423a30f, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_DIN, CONFIGURATION_HARDWARE_TYPE2_REL,  2, 1, 0);	// 3.1 - Łazienka - I piętro
	// 																																																																						// 3.2 - Korytarz - I piętro
	DeviceItem_AddItem(deviceItem, i++, 0xf12e11f2, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_DIN, CONFIGURATION_HARDWARE_TYPE2_REL,  2, 1, 0);	// 4.1 - Poddasze
																																																																							// 4.2 - <brak>
	DeviceItem_AddItem(deviceItem, i++, 0xe423a30f, LINE_UART2, CONFIGURATION_HARDWARE_TYPE1_DIN, CONFIGURATION_HARDWARE_TYPE2_REL,  2, 1, 0);	// 9.1 - CWU Boiler
																																																																							// 9.2 - CWU Instalacja
#endif
}

void DeviceItem_AddItem(struct DeviceItem *deviceItem, uint16_t i, uint32_t address,
		uint8_t lineNumber, uint8_t hardwareType1, uint8_t hardwareType2, uint8_t hardwareSegmentsCount,
		uint8_t hardwareVersion, uint16_t devicesItemsCount)
{
	struct DeviceItem *deviceItem_;
	deviceItem_ = &deviceItem[i];
	deviceItem_->address = address;
	deviceItem_->lineNumber = lineNumber;
	deviceItem_->hardwareType1 = hardwareType1;
	deviceItem_->hardwareType2 = hardwareType2;
	deviceItem_->hardwareSegmentsCount = hardwareSegmentsCount;
	deviceItem_->hardwareVersion = hardwareVersion;
	deviceItem_->devicesItems = NULL;
	deviceItem_->devicesItemsCount = devicesItemsCount;
	deviceItem_->deviceItemStatus = &devicesItemsStatus[i];
	deviceItem_->deviceItemControl = NULL;

	struct DeviceItemStatus *deviceItemStatus_;
	deviceItemStatus_ = &devicesItemsStatus[i];
	deviceItemStatus_->address = deviceItem_->address;
	deviceItemStatus_->deviceItem = deviceItem_;
	if (hardwareType2 == CONFIGURATION_HARDWARE_TYPE2_TEMP)
	{
		deviceItemStatus_->status = malloc(deviceItem_->hardwareSegmentsCount * sizeof(struct TemperatureStatus));
		memset(deviceItemStatus_->status, 0, deviceItem_->hardwareSegmentsCount * sizeof(struct TemperatureStatus));
	}
	else if (hardwareType2 == CONFIGURATION_HARDWARE_TYPE2_REL)
	{
		deviceItemStatus_->status = malloc(deviceItem_->hardwareSegmentsCount * sizeof(struct RelayStatus));
		memset(deviceItemStatus_->status, 0, deviceItem_->hardwareSegmentsCount * sizeof(struct RelayStatus));

		deviceItem_->deviceItemControl = malloc(deviceItem_->hardwareSegmentsCount * sizeof(struct RelayControl));
		memset(deviceItem_->deviceItemControl, 0, deviceItem_->hardwareSegmentsCount * sizeof(struct RelayControl));
	}
}

bool DeviceItem_GetDeviceItemFromAddress(uint32_t address, struct DeviceItem **deviceItem)
{
	uint16_t i = 0;
	struct DeviceItem *deviceItem_;
	while (i < devicesItems.devicesItemsCount)
	{
		deviceItem_ = &devicesItems.devicesItems[i++];
		if (deviceItem_->address == address)
		{
			*deviceItem = deviceItem_;
			return true;
		}
	}
	return false;
}

bool DeviceItem_GetDeviceStatusByAddress(uint32_t address, struct DeviceItemStatus **deviceItemStatus)
{
	uint16_t i = 0;
	struct DeviceItemStatus *deviceItemStatus_;
	while (i < devicesItems.devicesItemsCount)
	{
		deviceItemStatus_ = &devicesItemsStatus[i++];
		if (deviceItemStatus_->address == address)
		{
			*deviceItemStatus = deviceItemStatus_;
			return true;
		}
	}
	return false;
}

void DeviceItem_PrintDeviceItem(struct DeviceItem *deviceItem)
{
	LOGI(TAG, "DeviceItem:");
	LOGI(TAG, "  address: %.8x", deviceItem->address);
	LOGI(TAG, "  lineNumber: %d", deviceItem->lineNumber);
	LOGI(TAG, "  hardwareType1: %d", deviceItem->hardwareType1);
	LOGI(TAG, "  hardwareType2: %d", deviceItem->hardwareType2);
	LOGI(TAG, "  hardwareSegmentsCount: %d", deviceItem->hardwareSegmentsCount);
	LOGI(TAG, "  hardwareVersion: %d", deviceItem->hardwareVersion);
	LOGI(TAG, "  devicesItems: 0x%.8x", (uint32_t)deviceItem->devicesItems);
	LOGI(TAG, "  devicesItemsCount: %d", deviceItem->devicesItemsCount);
	LOGI(TAG, "  deviceItemStatus: %.8x", (uint32_t)deviceItem->deviceItemStatus);
}

void DeviceItem_PrintDeviceItemStatus(struct DeviceItemStatus *deviceItemStatus)
{
	LOGI(TAG, "DeviceItemStatus:");
	LOGI(TAG, "  address: %.8x", deviceItemStatus->address);
	LOGI(TAG, "  deviceItem: %.8x", (uint32_t)deviceItemStatus->deviceItem);
	LOGI(TAG, "  initialized: %d", deviceItemStatus->initialized);
	LOGI(TAG, "  error: %d", deviceItemStatus->error);
	LOGI(TAG, "  uptime: %d", deviceItemStatus->uptime);
	LOGI(TAG, "  vin: %.3f", deviceItemStatus->vin / 16.0);
	LOGI(TAG, "  communicationPercent: %d", deviceItemStatus->communicationPercent);
	LOGI(TAG, "  status: %.8x", (uint32_t)deviceItemStatus->status);

	if (deviceItemStatus->deviceItem->hardwareType2 == CONFIGURATION_HARDWARE_TYPE2_TEMP)
	{
		struct TemperatureStatus *temperatureStatus;
		temperatureStatus = devicesItemsStatus->status;
		for (uint8_t i = 0; i < deviceItemStatus->deviceItem->hardwareSegmentsCount; i++)
		{
			LOGI(TAG, "    temperature %d: %.3f", i + 1, temperatureStatus->temperature / 16.0);
			temperatureStatus++;
		}
	}
	else if (deviceItemStatus->deviceItem->hardwareType2 == CONFIGURATION_HARDWARE_TYPE2_REL)
	{
		struct RelayStatus *relayStatus;
		relayStatus = devicesItemsStatus->status;
		for (uint8_t i = 0; i < deviceItemStatus->deviceItem->hardwareSegmentsCount; i++)
		{
			LOGI(TAG, "    relay %d: %d", i + 1, relayStatus->relayState);
			relayStatus++;
		}
	}
}

void DeviceItem_InitializeConfigurationControls(struct FlashConfiguration *flashCfg,
		uint16_t *count, uint16_t *size)
{
	uint16_t i = 0;
	*count = 0;
	*size = 1 + 2;
	struct DeviceItem *deviceItem;
	while (i < devicesItems.devicesItemsCount)
	{
		deviceItem = &devicesItems.devicesItems[i++];
		if (deviceItem->hardwareType2 == CONFIGURATION_HARDWARE_TYPE2_REL)
		{
			*size += deviceItem->hardwareSegmentsCount * 1;
			(*count)++;
		}
	}

	if (*count == 0)
		return;

	cfgCuControl = malloc(*size);

	if (LogLevel >= 1)
	{
		LOGE(TAG, "Initialize cfgCuControl %d %d %x", *count, *size, (uint)cfgCuControl);
		PrintMemory(TAG, cfgCuControl, (uint)cfgCuControl, 64);
	}
}

void DeviceItem_WriteControlToConfiguration(struct FlashConfiguration *flashCfg)
{
	uint16_t o = 0;
	cfgCuControl[o++] = flashCfg->itemStoredSignature;
	cfgCuControl[o++] = UINT32_1BYTE(flashCfg->configurationSize);
	cfgCuControl[o++] = UINT32_0BYTE(flashCfg->configurationSize);

	uint16_t i = 0;
	struct DeviceItem *deviceItem;
	while (i < devicesItems.devicesItemsCount)
	{
		deviceItem = &devicesItems.devicesItems[i++];
		if (deviceItem->hardwareType2 == CONFIGURATION_HARDWARE_TYPE2_REL)
		{
			struct RelayControl *relayControl;
			relayControl = deviceItem->deviceItemControl;
			for (uint16_t j = 0; j < deviceItem->hardwareSegmentsCount; j++)
			{
				cfgCuControl[o++] = relayControl->relayState;
				relayControl++;
			}
		}
	}
}

bool DeviceItem_LoadControlFromFlash(struct FlashConfiguration *flashCfg)
{
	if (LogLevel >= 1)
		PrintFlashMemory("load", CONFIGURATION_CU_CONTROL_FLASH_ADDRESS, 128);

	if (flashCfg->configurationSize == 0)
		return true;

	uint16_t o = 0;
	if (cfgCuControl[o++] != flashCfg->itemStoredSignature)
		return false;

	uint16_t size = cfgCuControl[o++] << 8;
	size |= cfgCuControl[o++];
	if (size != flashCfg->configurationSize)
		return false;

	uint16_t i = 0;
	struct DeviceItem *deviceItem;
	while (i < devicesItems.devicesItemsCount)
	{
		deviceItem = &devicesItems.devicesItems[i++];
		if (deviceItem->hardwareType2 == CONFIGURATION_HARDWARE_TYPE2_REL)
		{
			struct RelayControl *relayControl;
			relayControl = deviceItem->deviceItemControl;
			for (uint16_t j = 0; j < deviceItem->hardwareSegmentsCount; j++)
			{
				relayControl->relayState = cfgCuControl[o++];
				if (o > size)
					return false;
				relayControl++;
			}
		}
	}

	if (LogLevel >= 1)
		LOGE(TAG, "load end ok o %d size %d", o, size);

	return o == size;
}

bool DeviceItem_WriteControlToFlash(struct FlashConfiguration *flashCfg)
{
	DeviceItem_WriteControlToConfiguration(flashCfg);
	WriteConfigurationToFlash(flashCfg);

	if (LogLevel >= 1)
	{
		LOGE(TAG, "Write cfgCuControl %x", (uint)cfgCuControl);
		PrintMemory(TAG, cfgCuControl, (uint)cfgCuControl, 64);
		PrintFlashMemory(TAG, CONFIGURATION_CU_CONTROL_FLASH_ADDRESS, 128);
	}

	return true;
}

uint16_t DeviceItem_GetStatus(struct DeviceItem *devicesItems, struct DeviceItemStatus *devicesItemsStatus,
		struct HeatingVisualComponent *heatingComponents,
		uint8_t *outBytes, uint16_t maxOutBytes, uint16_t fromItem, uint8_t details)
{
	uint16_t o = 0;
	struct DeviceItem *deviceItem;
	devicesItemsStatus += fromItem;

	uint8_t outItemsCount = 0;
	outBytes[o++] = outItemsCount;
	outBytes[o++] = UINT32_1BYTE(devicesItems->devicesItemsCount + heatingDevicesComponentsCount);
	outBytes[o++] = UINT32_0BYTE(devicesItems->devicesItemsCount + heatingDevicesComponentsCount);

	if (details >= 1)
	{
		outBytes[o++] = UINT32_3BYTE(uptime);
		outBytes[o++] = UINT32_2BYTE(uptime);
		outBytes[o++] = UINT32_1BYTE(uptime);
		outBytes[o++] = UINT32_0BYTE(uptime);
		outBytes[o++] = UINT32_1BYTE(vin);
		outBytes[o++] = UINT32_0BYTE(vin);
	}

	for (uint16_t i = fromItem; i < devicesItems->devicesItemsCount && o < maxOutBytes - 32; i++)
	{
		deviceItem = devicesItemsStatus->deviceItem;

		if (deviceItem->hardwareType2 == CONFIGURATION_HARDWARE_TYPE2_TEMP)
			outBytes[o++] = DEVICE_ITEM_STATUS_TYPE_TEMPERATURE;
		else if (deviceItem->hardwareType2 == CONFIGURATION_HARDWARE_TYPE2_REL)
			outBytes[o++] = DEVICE_ITEM_STATUS_TYPE_RELAY;
		else
			outBytes[o++] = 0;

		outBytes[o++] = UINT32_3BYTE(devicesItemsStatus->address);
		outBytes[o++] = UINT32_2BYTE(devicesItemsStatus->address);
		outBytes[o++] = UINT32_1BYTE(devicesItemsStatus->address);
		outBytes[o++] = UINT32_0BYTE(devicesItemsStatus->address);

		if (deviceItem->hardwareType2 == CONFIGURATION_HARDWARE_TYPE2_TEMP)
		{
			struct TemperatureStatus *temperatureStatus;
			temperatureStatus = devicesItemsStatus->status;
			outBytes[o++] = devicesItemsStatus->initialized;
			outBytes[o++] = devicesItemsStatus->error;
			if (details >= 1)
			{
				outBytes[o++] = UINT32_3BYTE(devicesItemsStatus->uptime);
				outBytes[o++] = UINT32_2BYTE(devicesItemsStatus->uptime);
				outBytes[o++] = UINT32_1BYTE(devicesItemsStatus->uptime);
				outBytes[o++] = UINT32_0BYTE(devicesItemsStatus->uptime);
				outBytes[o++] = UINT32_1BYTE(devicesItemsStatus->vin);
				outBytes[o++] = UINT32_0BYTE(devicesItemsStatus->vin);
				outBytes[o++] = UINT32_0BYTE(devicesItemsStatus->communicationPercent);
			}
			outBytes[o++] = deviceItem->hardwareSegmentsCount;
			for (uint16_t j = 0; j < deviceItem->hardwareSegmentsCount; j++)
			{
				if (LogLevel >= 1)
					LOGE(TAG, "  temp %.8x", (uint32_t)temperatureStatus);
				outBytes[o++] = UINT32_1BYTE(temperatureStatus->temperature);
				outBytes[o++] = UINT32_0BYTE(temperatureStatus->temperature);
				temperatureStatus++;
			}
		}
		else if (deviceItem->hardwareType2 == CONFIGURATION_HARDWARE_TYPE2_REL)
		{
			struct RelayStatus *relayStatus;
			relayStatus = devicesItemsStatus->status;
			outBytes[o++] = devicesItemsStatus->initialized;
			outBytes[o++] = devicesItemsStatus->error;
			if (details >= 1)
			{
				outBytes[o++] = UINT32_3BYTE(devicesItemsStatus->uptime);
				outBytes[o++] = UINT32_2BYTE(devicesItemsStatus->uptime);
				outBytes[o++] = UINT32_1BYTE(devicesItemsStatus->uptime);
				outBytes[o++] = UINT32_0BYTE(devicesItemsStatus->uptime);
				outBytes[o++] = UINT32_1BYTE(devicesItemsStatus->vin);
				outBytes[o++] = UINT32_0BYTE(devicesItemsStatus->vin);
				outBytes[o++] = UINT32_0BYTE(devicesItemsStatus->communicationPercent);
			}
			outBytes[o++] = deviceItem->hardwareSegmentsCount;
			for (uint16_t j = 0; j < deviceItem->hardwareSegmentsCount; j++)
			{
				if (LogLevel >= 1)
					LOGE(TAG, "  rel %.8x", (uint32_t)relayStatus);
				outBytes[o++] = relayStatus->relayState;
				relayStatus++;
			}
		}

		devicesItemsStatus++;
		outItemsCount++;
	}

	for (uint16_t i = 0; i < heatingDevicesComponentsCount && o < maxOutBytes - 32; i++)
		if (i + devicesItems->devicesItemsCount >= fromItem)
		{
			struct HeatingVisualComponentControl *heatingComponentControl;
			heatingComponentControl = heatingComponents->heatingVisualComponentControl;

			outBytes[o++] = DEVICE_ITEM_STATUS_TYPE_HEATING_VISUAL_COMPONENT;
			outBytes[o++] = UINT32_3BYTE(heatingComponents->deviceItem);
			outBytes[o++] = UINT32_2BYTE(heatingComponents->deviceItem);
			outBytes[o++] = UINT32_1BYTE(heatingComponents->deviceItem);
			outBytes[o++] = UINT32_0BYTE(heatingComponents->deviceItem);
			outBytes[o++] = heatingComponents->deviceSegment;
			outBytes[o++] = heatingComponentControl->mode;
			
			float setTemperature = NAN;
			if (heatingComponentControl->mode == HEATING_VISUAL_COMPONENT_MODE_AUTO && RTC_isSetRtc)
				setTemperature = CentralUnitScanDevices_GetSettingTemperature(heatingComponentControl);
			else if (heatingComponentControl->mode == HEATING_VISUAL_COMPONENT_MODE_MANUAL)
				setTemperature = heatingComponentControl->manualTemperature;
			outBytes[o++] = UINT32_1BYTE((int)(setTemperature * 16.0));
			outBytes[o++] = UINT32_0BYTE((int)(setTemperature * 16.0));

			heatingComponents++;
			outItemsCount++;
		}

	outBytes[0] = outItemsCount;
	return o;
}
#endif
