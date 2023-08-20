#include "../../include/main.h"
#if !defined(BOOTLOADER) && defined(ISR_CU)
#include <stdio.h>
#include "configuration_cu_control.h"

#define DEVICE_ITEM_STATUS_TYPE_TEMPERATURE               0x40
#define DEVICE_ITEM_STATUS_TYPE_RELAY                     0x41
#define DEVICE_ITEM_STATUS_TYPE_HEATING_VISUAL_COMPONENT  0x50

struct DeviceItem
{
	uint32_t address;
	uint8_t lineNumber;
	uint8_t hardwareType1;
	uint8_t hardwareType2;
	uint8_t hardwareSegmentsCount;
	uint8_t hardwareVersion;
	struct DeviceItem *devicesItems;
	uint16_t devicesItemsCount;
	struct DeviceItemStatus *deviceItemStatus;
	void *deviceItemControl; // RelayControl
};

struct DeviceItemStatus
{
	uint32_t address;
	struct DeviceItem *deviceItem;
	bool initialized;
	bool error;
	uint32_t errorFrom;
	uint32_t uptime;
	uint16_t vin;
	uint8_t communicationPercent;
	void *status;			/// RelayStatus or TemperatureStatus
};

struct RelayStatus
{
	bool relayState;
};

struct TemperatureStatus
{
	uint16_t temperature;
	bool error;
	uint32_t errorFrom;
};

struct RelayControl
{
	bool relayState;
	uint32_t setTime;			/// milliseconds
	uint32_t updateTime;	/// milliseconds
};

extern struct DeviceItem devicesItems;
extern struct DeviceItemStatus *devicesItemsStatus;

extern void DeviceItem_Initialize(void);
extern void DeviceItem_AddItem(struct DeviceItem *deviceItem, uint16_t i, uint32_t address,
		uint8_t lineNumber, uint8_t hardwareType1, uint8_t hardwareType2, uint8_t hardwareSegmentsCount,
		uint8_t hardwareVersion, uint16_t devicesItemsCount);
extern bool DeviceItem_GetDeviceItemFromAddress(uint32_t address, struct DeviceItem **deviceItem);
extern bool DeviceItem_GetDeviceStatusByAddress(uint32_t address, struct DeviceItemStatus **deviceItemStatus);
extern void DeviceItem_PrintDeviceItem(struct DeviceItem *deviceItem);
extern void DeviceItem_PrintDeviceItemStatus(struct DeviceItemStatus *deviceItemStatus);
extern void DeviceItem_InitializeConfigurationControls(struct FlashConfiguration *flashCfg,
		uint16_t *count, uint16_t *size);
extern void DeviceItem_WriteControlToConfiguration(struct FlashConfiguration *flashCfg);
extern bool DeviceItem_LoadControlFromFlash(struct FlashConfiguration *flashCfg);
extern bool DeviceItem_WriteControlToFlash(struct FlashConfiguration *flashCfg);
extern uint16_t DeviceItem_GetStatus(struct DeviceItem *devicesItems, struct DeviceItemStatus *devicesItemsStatus,
		struct HeatingVisualComponent *heatingComponents,
		uint8_t *outBytes, uint16_t maxOutBytes, uint16_t fromItem, uint8_t details);
#endif
