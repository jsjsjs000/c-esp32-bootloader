#include <stdio.h>
#include <string.h>
#include <unity.h>
#include "main.h"
#include "common.h"
#include "configuration.h"
#include "configuration_cu_control.h"
#include "configuration_visual_components.h"
#include "device_item.h"
#include "device_item2.h"
#include "flash.h"
#include "history_state.h"

static const char *TAG = "TEST";

uint32_t uptime = 0;      /// s

void test_a(void)
{
	DeviceItem_Initialize();
	VisualComponent_Initialize();
	// InitializeFlashConfigurationCuControl(&flashConfigurationCuControl);
	// InitializeFlashConfigurationVisualComponents(&flashConfigurationVisualComponents);
	// if (FindConfigurationInFlash(&flashConfigurationCuControl) != FIND_OK)
	// {
	// 		/// No configuration found - initialize it
	// 	if (WriteConfigurationToFlash(&flashConfigurationCuControl) != STATUS_OK)
	// 		ERROR_HANDLER();
	// }
	// if (FindConfigurationInFlash(&flashConfigurationVisualComponents) != FIND_OK)
	// {
	// 		/// No configuration found - initialize it
	// 	EraseFlashVisualComponents();
	// 	if (WriteConfigurationToFlash(&flashConfigurationVisualComponents) != STATUS_OK)
	// 		ERROR_HANDLER();
	// }
	// PrintFlashCuControlConfiguration(cfgCuControl);
	// PrintFlashVisualComponentsConfiguration(cfgVisualComponents);

	uint8_t txBuffer[1024];
	uint16_t fromItem = 0;
	uint16_t maxOutBytes = sizeof(txBuffer);
	bool details = true;
	uint16_t bytes = DeviceItem_GetStatus(&devicesItems, devicesItemsStatus, heatingDevicesComponents,
			txBuffer, maxOutBytes, fromItem, details);
	LOGI("MEMORY", "bytes %d", bytes);
	PrintMemory("MEMORY", txBuffer, 0, 384);

	// uint16_t i = 0;
	// struct DeviceItem *deviceItem;
	// while (i < devicesItems.devicesItemsCount)
	// {
	// 	deviceItem = &devicesItems.devicesItems[i++];
	// 	DeviceItem_PrintDeviceItem(deviceItem);
	// 	DeviceItem_PrintDeviceItemStatus(deviceItem->deviceItemStatus);
	// }

	TEST_ASSERT_EQUAL(3, 3);
}

void test_flash(void)
{
	uint32_t flash = 0x380000;
	PrintFlashMemory("MEMORY1", flash, 16);
	TEST_ASSERT_EQUAL(STATUS_OK, EraseFlashRegion(flash, FLASH_PAGE_SIZE));
	PrintFlashMemory("MEMORY2", flash, 16);
	
	uint8_t write1[] = { 0x12, 0xff, 0x56, 0x78 };
	TEST_ASSERT_EQUAL(STATUS_OK, WriteToFlash(flash, sizeof(write1), write1));
	PrintFlashMemory("MEMORY3", flash, 16);

	uint8_t write2[] = { 0xab };
	TEST_ASSERT_EQUAL(STATUS_OK, WriteToFlash(flash + 1, sizeof(write2), write2));
	PrintFlashMemory("MEMORY4", flash, 16);
}

uint32_t check_FindHistoryStateInFlash(struct FlashHistoryState* flashCfg)
{
	uint32_t address = flashCfg->currentFlashAddress;
	if (FindHistoryStateInFlash(flashCfg) != FIND_OK)
	{
		flashCfg->currentFlashAddress = address;
		return 0;
	}

	uint32_t returnAddress = flashCfg->currentFlashAddress;
	flashCfg->currentFlashAddress = address;
	return returnAddress;
}

void test_history_state(void)
{
	TEST_ASSERT_EQUAL(STATUS_OK,
			EraseFlashRegion(FLASH_HISTORY_STATE_FLASH_ADDRESS, FLASH_PAGE_SIZE));

	uint8_t trashData[] = { 0x45, 0x43, 0x34, 0x65, 0x13, 0x63, 0x76, 0x98, 0x88,
			0x44, 0x33, 0x11, 0x88, 0x95, 0x44, 0x43, 0x12 };
	TEST_ASSERT_EQUAL(STATUS_OK,
			EraseFlashRegion(FLASH_HISTORY_STATE_FLASH_ADDRESS + FLASH_PAGE_SIZE, FLASH_PAGE_SIZE));
	TEST_ASSERT_EQUAL(STATUS_OK,
			WriteToFlash(FLASH_HISTORY_STATE_FLASH_ADDRESS + FLASH_PAGE_SIZE, FLASH_PAGE_SIZE, trashData));

	TEST_ASSERT_EQUAL(STATUS_OK,
			EraseFlashRegion(FLASH_HISTORY_STATE_FLASH_ADDRESS + FLASH_HISTORY_STATE_FLASH_SIZE - FLASH_PAGE_SIZE,
				FLASH_PAGE_SIZE));
	TEST_ASSERT_EQUAL(STATUS_OK,
			WriteToFlash(FLASH_HISTORY_STATE_FLASH_ADDRESS + FLASH_HISTORY_STATE_FLASH_SIZE - FLASH_PAGE_SIZE,
				FLASH_PAGE_SIZE, trashData));

	InitializeFlashHistoryState(&flashHistoryState);

		/// tymczasowy mniejszy pozycja history state
	flashHistoryState.configurationSize = 5;

		/// find first free item
	TEST_ASSERT_TRUE(FindHistoryStateInFlash(&flashHistoryState) == FIND_OK);
	TEST_ASSERT_EQUAL(FLASH_HISTORY_STATE_FLASH_ADDRESS, flashHistoryState.currentFlashAddress);

		/// write first item
	uint8_t item1[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
	TEST_ASSERT_EQUAL(STATUS_OK, WriteHistoryStateToFlash(&flashHistoryState, item1));
	PrintFlashMemory("MEMORY1", FLASH_HISTORY_STATE_FLASH_ADDRESS, 16);
	uint8_t memory1[] = { FLASH_HISTORY_STATE_ITEM_STORED_SIGNATURE, 0xff,
			0x01, 0x02, 0x03, 0x04, 0x05, 0xff, 0xff };
	TEST_ASSERT_TRUE(CompareMemory(TAG, FLASH_HISTORY_STATE_FLASH_ADDRESS, memory1, sizeof(memory1)));

		/// find first free item - after first item
	TEST_ASSERT_TRUE(FindHistoryStateInFlash(&flashHistoryState) == FIND_OK);
	TEST_ASSERT_EQUAL(FLASH_HISTORY_STATE_FLASH_ADDRESS + GetHistoryStateItemSize(&flashHistoryState),
			flashHistoryState.currentFlashAddress);

		/// write second item
	uint8_t item2[] = { 0x11, 0x12, 0x13, 0x14, 0x15 };
	TEST_ASSERT_EQUAL(STATUS_OK, WriteHistoryStateToFlash(&flashHistoryState, item2));
	PrintFlashMemory("MEMORY2", FLASH_HISTORY_STATE_FLASH_ADDRESS, 32);
	uint8_t memory2[] = { FLASH_HISTORY_STATE_ITEM_STORED_SIGNATURE, 0xff,
			0x01, 0x02, 0x03, 0x04, 0x05, 
			FLASH_HISTORY_STATE_ITEM_STORED_SIGNATURE, 0xff,
			0x11, 0x12, 0x13, 0x14, 0x15, 0xff, 0xff };
	TEST_ASSERT_TRUE(CompareMemory(TAG, FLASH_HISTORY_STATE_FLASH_ADDRESS, memory2, sizeof(memory2)));
	PrintFlashMemory("MEMORY3", FLASH_HISTORY_STATE_FLASH_ADDRESS, 16);

		/// find first free item - after second item
	TEST_ASSERT_TRUE(FindHistoryStateInFlash(&flashHistoryState) == FIND_OK);
	TEST_ASSERT_EQUAL(FLASH_HISTORY_STATE_FLASH_ADDRESS + 2 * GetHistoryStateItemSize(&flashHistoryState),
			flashHistoryState.currentFlashAddress);

		/// write all first flash sector - 4096 bytes
	PrintFlashMemory("MEMORY4", FLASH_HISTORY_STATE_FLASH_ADDRESS + FLASH_PAGE_SIZE - 16, 32);
	for (uint32_t i = 0; i < 586 - 2; i++)
	{
		TEST_ASSERT_EQUAL(STATUS_OK, WriteHistoryStateToFlash(&flashHistoryState, item1));
		TEST_ASSERT_EQUAL(
				FLASH_HISTORY_STATE_FLASH_ADDRESS + (i + 3) * GetHistoryStateItemSize(&flashHistoryState),
				check_FindHistoryStateInFlash(&flashHistoryState));
	}
	PrintFlashMemory("MEMORY5", FLASH_HISTORY_STATE_FLASH_ADDRESS + FLASH_PAGE_SIZE - 16, 32);

		/// test end flash - 480 * 1024 = 491520 / 7 = 70217 - 50 seconds long
	const uint32_t ItemsInFlash = 480 * 1024 / GetHistoryStateItemSize(&flashHistoryState);
	LOGI(TAG, "test end of flash");
	for (uint32_t i = 0; i < ItemsInFlash - 3 - (586 - 2); i++)
		TEST_ASSERT_EQUAL(STATUS_OK, WriteHistoryStateToFlash(&flashHistoryState, item1));
	PrintFlashMemory("MEMORY6", FLASH_HISTORY_STATE_FLASH_ADDRESS, 32);
	PrintFlashMemory("MEMORY7", FLASH_HISTORY_STATE_FLASH_ADDRESS + FLASH_HISTORY_STATE_FLASH_SIZE - 32, 32);
	LOGI(TAG, "current address = %x\n", flashHistoryState.currentFlashAddress);

	TEST_ASSERT_EQUAL(
				FLASH_HISTORY_STATE_FLASH_ADDRESS + (ItemsInFlash - 1) * GetHistoryStateItemSize(&flashHistoryState),
				check_FindHistoryStateInFlash(&flashHistoryState));

	TEST_ASSERT_EQUAL(STATUS_OK, WriteHistoryStateToFlash(&flashHistoryState, item1));
	PrintFlashMemory("MEMORY8", FLASH_HISTORY_STATE_FLASH_ADDRESS, 32);
	PrintFlashMemory("MEMORY9", FLASH_HISTORY_STATE_FLASH_ADDRESS + FLASH_HISTORY_STATE_FLASH_SIZE - 32, 32);
	LOGI(TAG, "current address = %x\n", flashHistoryState.currentFlashAddress);

	TEST_ASSERT_EQUAL(FLASH_HISTORY_STATE_FLASH_ADDRESS, check_FindHistoryStateInFlash(&flashHistoryState));

		/// first item at begin of flash
	TEST_ASSERT_EQUAL(STATUS_OK, WriteHistoryStateToFlash(&flashHistoryState, item1));
	PrintFlashMemory("MEMORY10", FLASH_HISTORY_STATE_FLASH_ADDRESS, 32);
	LOGI(TAG, "current address = %x\n", flashHistoryState.currentFlashAddress);

	TEST_ASSERT_EQUAL(FLASH_HISTORY_STATE_FLASH_ADDRESS + GetHistoryStateItemSize(&flashHistoryState),
			check_FindHistoryStateInFlash(&flashHistoryState));
}

uint32_t check_FindNotSendHistoryState(struct FlashHistoryState* flashCfg)
{
	uint32_t address = flashCfg->notSendCurrentItemAddress;
	if (!FindNotSendHistoryState(flashCfg))
	{
		flashCfg->notSendCurrentItemAddress = address;
		return 0;
	}

	uint32_t returnAddress = flashCfg->notSendCurrentItemAddress;
	flashCfg->notSendCurrentItemAddress = address;
	return returnAddress;
}

void test_history_state_synchronize_to_server(void)
{
	int itemSize = GetHistoryStateItemSize(&flashHistoryState);
	int itemIndex = 0;

		/// erase first page
	TEST_ASSERT_EQUAL(STATUS_OK,
			EraseFlashRegion(FLASH_HISTORY_STATE_FLASH_ADDRESS, FLASH_PAGE_SIZE));

		/// erase second page
	TEST_ASSERT_EQUAL(STATUS_OK,
			EraseFlashRegion(FLASH_HISTORY_STATE_FLASH_ADDRESS + FLASH_PAGE_SIZE, FLASH_PAGE_SIZE));
	uint8_t trashData[] = { 0x45, 0x43, 0x34, 0x65, 0x13, 0x63, 0x76, 0x98, 0x88,
			0x44, 0x33, 0x11, 0x88, 0x95, 0x44, 0x43, 0x12 };
		/// write trash data to second page
	TEST_ASSERT_EQUAL(STATUS_OK,
			WriteToFlash(FLASH_HISTORY_STATE_FLASH_ADDRESS + FLASH_PAGE_SIZE, FLASH_PAGE_SIZE, trashData));

	InitializeFlashHistoryState(&flashHistoryState);

		/// find first free item
	TEST_ASSERT_TRUE(FindHistoryStateInFlash(&flashHistoryState) == FIND_OK);
	TEST_ASSERT_EQUAL(FLASH_HISTORY_STATE_FLASH_ADDRESS, flashHistoryState.currentFlashAddress);
LOGE(TAG, "A1 - cur %x, not_send %x", flashHistoryState.currentFlashAddress, flashHistoryState.notSendCurrentItemAddress);

	TEST_ASSERT_EQUAL(0, check_FindNotSendHistoryState(&flashHistoryState));

		/// prepare item
	uint8_t item[itemSize];
	for (int i = 0; i < itemSize; i++)
		item[i] = i * 13;

		/// write first item
	//uint8_t item1[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
	item[0] = itemIndex >> 8;
	item[1] = itemIndex++ & 0xff;
	TEST_ASSERT_EQUAL(STATUS_OK, WriteHistoryStateToFlash(&flashHistoryState, item));
	PrintFlashMemory(TAG, FLASH_HISTORY_STATE_FLASH_ADDRESS, 16);
LOGE(TAG, "B1 - cur %x, not_send %x", flashHistoryState.currentFlashAddress, flashHistoryState.notSendCurrentItemAddress);

	uint32_t address = check_FindNotSendHistoryState(&flashHistoryState);
	TEST_ASSERT_EQUAL(FLASH_HISTORY_STATE_FLASH_ADDRESS, address);
	PrintFlashMemory(TAG, FLASH_HISTORY_STATE_FLASH_ADDRESS, 16);
LOGE(TAG, "B2 - cur %x, not_send %x", flashHistoryState.currentFlashAddress, flashHistoryState.notSendCurrentItemAddress);

	TEST_ASSERT_EQUAL(STATUS_OK, SetSendHistoryState(&flashHistoryState));
	PrintFlashMemory(TAG, FLASH_HISTORY_STATE_FLASH_ADDRESS, 16);
LOGE(TAG, "B3 - cur %x, not_send %x", flashHistoryState.currentFlashAddress, flashHistoryState.notSendCurrentItemAddress);

	address = check_FindNotSendHistoryState(&flashHistoryState);
	TEST_ASSERT_EQUAL(0, address);
LOGE(TAG, "B4 - cur %x, not_send %x", flashHistoryState.currentFlashAddress, flashHistoryState.notSendCurrentItemAddress);

		/// write second item
	// uint8_t item2[] = { 0x11, 0x12, 0x13, 0x14, 0x15 };
	item[0] = itemIndex >> 8;
	item[1] = itemIndex++ & 0xff;
	TEST_ASSERT_EQUAL(STATUS_OK, WriteHistoryStateToFlash(&flashHistoryState, item));
	PrintFlashMemory(TAG, FLASH_HISTORY_STATE_FLASH_ADDRESS, 32);
LOGE(TAG, "C1 - cur %x, not_send %x", flashHistoryState.currentFlashAddress, flashHistoryState.notSendCurrentItemAddress);

	address = check_FindNotSendHistoryState(&flashHistoryState);
	TEST_ASSERT_EQUAL(FLASH_HISTORY_STATE_FLASH_ADDRESS + GetHistoryStateItemSize(&flashHistoryState),
			address);
	PrintFlashMemory(TAG, FLASH_HISTORY_STATE_FLASH_ADDRESS, 32);
LOGE(TAG, "C2 - cur %x, not_send %x", flashHistoryState.currentFlashAddress, flashHistoryState.notSendCurrentItemAddress);

	TEST_ASSERT_EQUAL(STATUS_OK, SetSendHistoryState(&flashHistoryState));
	PrintFlashMemory(TAG, FLASH_HISTORY_STATE_FLASH_ADDRESS, 32);
LOGE(TAG, "C3 - cur %x, not_send %x", flashHistoryState.currentFlashAddress, flashHistoryState.notSendCurrentItemAddress);

	address = check_FindNotSendHistoryState(&flashHistoryState);
	TEST_ASSERT_EQUAL(0, address);
LOGE(TAG, "C4 - cur %x, not_send %x", flashHistoryState.currentFlashAddress, flashHistoryState.notSendCurrentItemAddress);

		/// write 3rd and 4th items
	item[0] = itemIndex >> 8;
	item[1] = itemIndex++ & 0xff;
	TEST_ASSERT_EQUAL(STATUS_OK, WriteHistoryStateToFlash(&flashHistoryState, item));
	item[0] = itemIndex >> 8;
	item[1] = itemIndex++ & 0xff;
	TEST_ASSERT_EQUAL(STATUS_OK, WriteHistoryStateToFlash(&flashHistoryState, item));
	PrintFlashMemory(TAG, FLASH_HISTORY_STATE_FLASH_ADDRESS, 64);
LOGE(TAG, "D1 - cur %x, not_send %x", flashHistoryState.currentFlashAddress, flashHistoryState.notSendCurrentItemAddress);

	address = check_FindNotSendHistoryState(&flashHistoryState);
	TEST_ASSERT_EQUAL(FLASH_HISTORY_STATE_FLASH_ADDRESS + 2 * GetHistoryStateItemSize(&flashHistoryState),
			address);
	PrintFlashMemory(TAG, FLASH_HISTORY_STATE_FLASH_ADDRESS, 64);
LOGE(TAG, "D2 - cur %x, not_send %x", flashHistoryState.currentFlashAddress, flashHistoryState.notSendCurrentItemAddress);

	TEST_ASSERT_EQUAL(STATUS_OK, SetSendHistoryState(&flashHistoryState));
	PrintFlashMemory(TAG, FLASH_HISTORY_STATE_FLASH_ADDRESS, 64);
LOGE(TAG, "D3 - cur %x, not_send %x", flashHistoryState.currentFlashAddress, flashHistoryState.notSendCurrentItemAddress);

	address = check_FindNotSendHistoryState(&flashHistoryState);
	TEST_ASSERT_EQUAL(FLASH_HISTORY_STATE_FLASH_ADDRESS + 3 * GetHistoryStateItemSize(&flashHistoryState),
			address);
	PrintFlashMemory(TAG, FLASH_HISTORY_STATE_FLASH_ADDRESS, 64);
LOGE(TAG, "D4 - cur %x, not_send %x", flashHistoryState.currentFlashAddress, flashHistoryState.notSendCurrentItemAddress);

	TEST_ASSERT_EQUAL(STATUS_OK, SetSendHistoryState(&flashHistoryState));
	PrintFlashMemory(TAG, FLASH_HISTORY_STATE_FLASH_ADDRESS, 64);
LOGE(TAG, "D5 - cur %x, not_send %x", flashHistoryState.currentFlashAddress, flashHistoryState.notSendCurrentItemAddress);

		/// write rest of flash
		/// 480 * 1024 / 370 = 1328, 1328 - 4 = 1324
	for (int i = 0; i < 480 * 1024 / GetHistoryStateItemSize(&flashHistoryState) - 4; i++)
	{
		item[0] = itemIndex >> 8;
		item[1] = itemIndex++ & 0xff;
		TEST_ASSERT_EQUAL(STATUS_OK, WriteHistoryStateToFlash(&flashHistoryState, item));
// LOGE(TAG, "D5 - cur %x, not_send %x", flashHistoryState.currentFlashAddress, flashHistoryState.notSendCurrentItemAddress);
	}
LOGE(TAG, "end loop");

		/// write few items to overflow history states
	for (int i = 0; i < 4 + 1; i++)
	{
		item[0] = itemIndex >> 8;
		item[1] = itemIndex++ & 0xff;
		TEST_ASSERT_EQUAL(STATUS_OK, WriteHistoryStateToFlash(&flashHistoryState, item));
LOGE(TAG, "Item %d - cur %x, not_send %x", itemIndex, flashHistoryState.currentFlashAddress, flashHistoryState.notSendCurrentItemAddress);
	}

		/// read all history states
	do
	{
		address = check_FindNotSendHistoryState(&flashHistoryState);
// LOGE(TAG, "E1 - cur %x, not_send %x", flashHistoryState.currentFlashAddress, flashHistoryState.notSendCurrentItemAddress);

		if (address != 0)
		{
			uint16_t buffer;
			TEST_ASSERT_EQUAL(STATUS_OK, ReadFromFlash(address, 2, (uint8_t*)&buffer));
// LOGE(TAG, "E2 - read item index %x", buffer);

			TEST_ASSERT_EQUAL(STATUS_OK, SetSendHistoryState(&flashHistoryState));
// LOGE(TAG, "E3 - cur %x, not_send %x", flashHistoryState.currentFlashAddress, flashHistoryState.notSendCurrentItemAddress);
		}
	}
	while (address != 0);
}

void app_main(void)
{
	UNITY_BEGIN();
	// RUN_TEST(test_a);
	// RUN_TEST(test_flash);
	// RUN_TEST(test_history_state);
	RUN_TEST(test_history_state_synchronize_to_server);
	UNITY_END();
}

// data2, i, 0x78000, # 480KB = 120 * 4KB
// esp_flash_erase_region 4096 bytes - 29 ms
// esp_flash_erase_region 40960 bytes - 270 ms
// esp_flash_write 4096 - 8 ms
// esp_flash_write 10 * 4096 - 84 ms

// 369 DeviceItem_GetStatus, 23*16=368, 24*16=384
