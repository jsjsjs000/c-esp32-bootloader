#include "../../include/main.h"
#if !defined(BOOTLOADER) && defined(ISR_CU)
#include <stdio.h>

#define CU_SYNCHRONIZATION_TIME            (60 * 1000)
#define CU_SYNCHRONIZATION_STARTUP_DELAY1  4000
#define CU_SYNCHRONIZATION_STARTUP_DELAY2  6000
#define CU_SYNCHRONIZATION_ERROR_TIMEOUT   (15 * 1000)

struct CentralUnitScanDevices
{
	bool isAnyDeviceItemToScan;
	bool isFirstSynchronizationSend;
	uint32_t lastSynchronizationSend;
	uint32_t startScanCycle;
	uint16_t deviceItemIndex;
};

extern void CentralUnitScanDevices_Initialize(struct Communication *communication,
		struct CentralUnitScanDevices *scanDevices);
extern float CentralUnitScanDevices_GetSettingTemperature(struct HeatingVisualComponentControl *heatingComponentControl);
extern bool CentralUnitScanDevices_SendDevicesStates(struct Communication *communication);
extern void CentralUnitScanDevices_Receive(struct Communication *communication);
extern bool CentralUnitScanDevices_Loop(struct Communication *communication,
		struct CentralUnitScanDevices *scanDevices);
#endif
