#include "../../include/main.h"
#if !defined(BOOTLOADER) && defined(ISR_CU)
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "device_item2.h"
#include "configuration_bootloader.h"
#include "configuration_visual_components.h"
#include "communication.h"
#include "../ISR_ESP32_Library/adc.h"

static const char *TAG = "HeatingVisualComponent";
static const uint8_t LogLevel = 0; /// 0 - 1

struct HeatingVisualComponent *heatingDevicesComponents;
uint16_t heatingDevicesComponentsCount = 0;

void VisualComponent_Initialize(void)
{
#ifdef CONFIGURATION_TEST
	heatingDevicesComponentsCount = 2;
#endif
#if defined(CONFIGURATION_ADAM_KUKUC) || defined(CONFIGURATION_ADAM_KUKUC_TEST)
	heatingDevicesComponentsCount = 9;
#endif

	heatingDevicesComponents = (struct HeatingVisualComponent*)malloc(
			heatingDevicesComponentsCount * sizeof(struct HeatingVisualComponent));
	memset(heatingDevicesComponents, 0, heatingDevicesComponentsCount * sizeof(struct HeatingVisualComponent));

	uint16_t i = 0;
	uint16_t j;
	struct HeatingVisualComponent *hvc;

#ifdef CONFIGURATION_TEST
	j = 0;
	hvc = VisualComponent_AddItem(i++, 0xe423a30f, 0, 2);	//	Pokój 1 (relay 0):
	VisualComponent_AddSubItem(hvc, j++, 0x66a8c8e8, 0);		//		Podłoga
	VisualComponent_AddSubItem(hvc, j++, 0x66a8c8e8, 1);		//		Powietrze
	j = 0;
	hvc = VisualComponent_AddItem(i++, 0xe423a30f, 1, 2);	//	Pokój 2 (relay 1):
	VisualComponent_AddSubItem(hvc, j++, 0x262f6efd, 0);		//		Podłoga
	VisualComponent_AddSubItem(hvc, j++, 0x262f6efd, 1);		//		Powietrze
#endif
#if defined(CONFIGURATION_ADAM_KUKUC) || defined(CONFIGURATION_ADAM_KUKUC_TEST)
	j = 0;																								//	Parter:
	hvc = VisualComponent_AddItem(i++, 0xbd2fa348, 0, 2);	//		Salon i kuchnia - parter (relay 0):
	VisualComponent_AddSubItem(hvc, j++, 0x74024593, 1);	//			Podłoga
	VisualComponent_AddSubItem(hvc, j++, 0x74024593, 0);	//			Powietrze
	j = 0;
	hvc = VisualComponent_AddItem(i++, 0xbd2fa348, 1, 2);	//		Korytarz, wejście, klatka schodowa - parter (relay 1):
	VisualComponent_AddSubItem(hvc, j++, 0xedd60678, 1);	//			Podłoga
	VisualComponent_AddSubItem(hvc, j++, 0xedd60678, 0);	//			Powietrze
	j = 0;
	hvc = VisualComponent_AddItem(i++, 0xbc9ebdef, 0, 2);	//		WC - parter (relay 0):
	VisualComponent_AddSubItem(hvc, j++, 0xda906314, 1);	//			Podłoga
	VisualComponent_AddSubItem(hvc, j++, 0xda906314, 0);	//			Powietrze
	j = 0;																								//	I piętro:
	hvc = VisualComponent_AddItem(i++, 0xbc9ebdef, 1, 2);	//		Sypialnia - I piętro (relay 1):
	VisualComponent_AddSubItem(hvc, j++, 0x6efa00b0, 1);	//			Podłoga
	VisualComponent_AddSubItem(hvc, j++, 0x6efa00b0, 0);	//			Powietrze
#ifndef CONFIGURATION_ADAM_KUKUC_TEST
	j = 0;
	hvc = VisualComponent_AddItem(i++, 0xe423a30f, 0, 2);	//		Łazienka - I piętro (relay 0):
	VisualComponent_AddSubItem(hvc, j++, 0x66a8c8e8, 1);	//			Podłoga
	VisualComponent_AddSubItem(hvc, j++, 0x66a8c8e8, 0);	//			Powietrze
	j = 0;
	hvc = VisualComponent_AddItem(i++, 0xe423a30f, 1, 2);	//		Korytarz, klatka schodowa - I piętro (relay 1):
	VisualComponent_AddSubItem(hvc, j++, 0x262f6efd, 1);	//			Podłoga
	VisualComponent_AddSubItem(hvc, j++, 0x262f6efd, 0);	//			Powietrze
#endif
	j = 0;																								//	Poddasze:
	hvc = VisualComponent_AddItem(i++, 0xf12e11f2, 0, 2);	//		Poddasze (relay 0):
	VisualComponent_AddSubItem(hvc, j++, 0x1b28c309, 1);	//			Podłoga
	VisualComponent_AddSubItem(hvc, j++, 0x1b28c309, 0);	//			Powietrze
#endif
#ifdef CONFIGURATION_ADAM_KUKUC
	j = 0;																								//	CWU
	hvc = VisualComponent_AddItem(i++, 0x7bf90ca3, 0, 1);	//		CWU Boiler (relay 0):
	VisualComponent_AddSubItem(hvc, j++, 0xd054c0f1, 0);	//			Temperatura
	j = 0;
	hvc = VisualComponent_AddItem(i++, 0x7bf90ca3, 1, 1);	//		CWU Instalacja (relay 1):
	VisualComponent_AddSubItem(hvc, j++, 0xd054c0f1, 1);	//			Temperatura
#endif
#ifdef CONFIGURATION_ADAM_KUKUC_TEST
	j = 0;																								//	CWU
	hvc = VisualComponent_AddItem(i++, 0xe423a30f, 0, 1);	//		CWU Boiler (relay 0):
	VisualComponent_AddSubItem(hvc, j++, 0x262f6efd, 0);	//			Temperatura
	j = 0;
	hvc = VisualComponent_AddItem(i++, 0xe423a30f, 1, 1);	//		CWU Instalacja (relay 1):
	VisualComponent_AddSubItem(hvc, j++, 0x262f6efd, 1);	//			Temperatura
#endif
}

struct HeatingVisualComponent* VisualComponent_AddItem(uint16_t componentIndex,
		uint32_t address, uint8_t deviceSegment, uint16_t subItemsCount)
{
	struct HeatingVisualComponent *component;
	component = &heatingDevicesComponents[componentIndex];
	component->deviceItem = address;
	component->deviceSegment = deviceSegment;
	
	component->subComponentsCount = subItemsCount;
	component->subComponents = malloc(subItemsCount * sizeof(struct HeatingVisualComponentSubItem));
	
	memset(component->subComponents, 0, subItemsCount * sizeof(struct HeatingVisualComponentSubItem));
	component->heatingVisualComponentControl = malloc(sizeof(struct HeatingVisualComponentControl));
	VisualComponent_InitializeItemControl(component->heatingVisualComponentControl);

	return component;
}

void VisualComponent_AddSubItem(struct HeatingVisualComponent *component,
		uint16_t subItemIndex, uint32_t address, uint8_t deviceSegment)
{
	struct HeatingVisualComponentSubItem *subItem;
	subItem = component->subComponents;
	subItem = &subItem[subItemIndex];
	subItem->deviceItem = address;
	subItem->deviceSegment = deviceSegment;
}

uint8_t VisualComponent_TimeToByte(uint8_t hour, uint8_t minute)
{
	return hour * 10 + minute / 10;
}

uint8_t VisualComponent_TimeByteToHour(uint8_t time)
{
	return time / 10;
}

uint8_t VisualComponent_TimeByteToMinute(uint8_t time)
{
	return time - (uint8_t)(time / 10) * 10;
}

void VisualComponent_InitializeItemControl(struct HeatingVisualComponentControl *componentControl)
{
	componentControl->mode = HEATING_VISUAL_COMPONENT_MODE_OFF;
	componentControl->periodsPnPtCount = 2;
	componentControl->periodsSaCount = 1;
	componentControl->periodsSuCount = 1;
	componentControl->periodPnPtFrom[0] = VisualComponent_TimeToByte(6, 0);
	componentControl->periodPnPtFrom[1] = VisualComponent_TimeToByte(13, 0);
	componentControl->periodPnPtFrom[2] = VisualComponent_TimeToByte(15, 0);
	componentControl->periodPnPtFrom[3] = VisualComponent_TimeToByte(22, 0);
	componentControl->periodSaFrom[0] = VisualComponent_TimeToByte(6, 0);
	componentControl->periodSaFrom[1] = VisualComponent_TimeToByte(13, 0);
	componentControl->periodSaFrom[2] = VisualComponent_TimeToByte(15, 0);
	componentControl->periodSaFrom[3] = VisualComponent_TimeToByte(22, 0);
	componentControl->periodSuFrom[0] = VisualComponent_TimeToByte(6, 0);
	componentControl->periodSuFrom[1] = VisualComponent_TimeToByte(13, 0);
	componentControl->periodSuFrom[2] = VisualComponent_TimeToByte(15, 0);
	componentControl->periodSuFrom[3] = VisualComponent_TimeToByte(22, 0);
	componentControl->manualTemperature = 21.0;
	componentControl->periodPnPtTemperature[0] = 21.0;
	componentControl->periodPnPtTemperature[1] = 18.0;
	componentControl->periodPnPtTemperature[2] = 21.0;
	componentControl->periodPnPtTemperature[3] = 18.0;
	componentControl->periodSaTemperature[0] = 21.0;
	componentControl->periodSaTemperature[1] = 18.0;
	componentControl->periodSaTemperature[2] = 21.0;
	componentControl->periodSaTemperature[3] = 18.0;
	componentControl->periodSuTemperature[0] = 21.0;
	componentControl->periodSuTemperature[1] = 18.0;
	componentControl->periodSuTemperature[2] = 21.0;
	componentControl->periodSuTemperature[3] = 18.0;
	componentControl->maxFloorTemperature = 30.0;
	componentControl->hysteresisTemperature = 0.5;
}

void VisualComponent_InitializeConfigurationVisualComponents(struct FlashConfiguration *flashCfg,
		uint16_t *count, uint16_t *size)
{
	*count = heatingDevicesComponentsCount;
	*size = 1 + 2 + heatingDevicesComponentsCount * (1 + 3 + 2 + 3 * 4 * (1 + 2) + 2 + 2);
	cfgVisualComponents = malloc(*size);

	if (LogLevel >= 1)
	{
		LOGE(TAG, "Initialize cfgVisualComponents %d %x", *size, (uint)cfgVisualComponents);
		PrintMemory(TAG, cfgVisualComponents, (uint)cfgVisualComponents, 64);
	}
}

void VisualComponent_WriteVisualComponentsToConfiguration(struct FlashConfiguration *flashCfg)
{
	uint16_t o = 0;
	cfgVisualComponents[o++] = flashCfg->itemStoredSignature;
	cfgVisualComponents[o++] = UINT32_1BYTE(flashCfg->configurationSize);
	cfgVisualComponents[o++] = UINT32_0BYTE(flashCfg->configurationSize);

	struct HeatingVisualComponent *component;
	struct HeatingVisualComponentControl *componentControl;
	for (uint16_t i = 0; i < heatingDevicesComponentsCount; i++)
	{
		component = &heatingDevicesComponents[i];
		componentControl = component->heatingVisualComponentControl;
		cfgVisualComponents[o++] = componentControl->mode;
		cfgVisualComponents[o++] = componentControl->periodsPnPtCount;
		cfgVisualComponents[o++] = componentControl->periodsSaCount;
		cfgVisualComponents[o++] = componentControl->periodsSuCount;
		cfgVisualComponents[o++] = componentControl->periodPnPtFrom[0];
		cfgVisualComponents[o++] = componentControl->periodPnPtFrom[1];
		cfgVisualComponents[o++] = componentControl->periodPnPtFrom[2];
		cfgVisualComponents[o++] = componentControl->periodPnPtFrom[3];
		cfgVisualComponents[o++] = componentControl->periodSaFrom[0];
		cfgVisualComponents[o++] = componentControl->periodSaFrom[1];
		cfgVisualComponents[o++] = componentControl->periodSaFrom[2];
		cfgVisualComponents[o++] = componentControl->periodSaFrom[3];
		cfgVisualComponents[o++] = componentControl->periodSuFrom[0];
		cfgVisualComponents[o++] = componentControl->periodSuFrom[1];
		cfgVisualComponents[o++] = componentControl->periodSuFrom[2];
		cfgVisualComponents[o++] = componentControl->periodSuFrom[3];
		cfgVisualComponents[o++] = UINT32_1BYTE((uint16_t)(componentControl->manualTemperature * 10.0));
		cfgVisualComponents[o++] = UINT32_0BYTE((uint16_t)(componentControl->manualTemperature * 10.0));
 		cfgVisualComponents[o++] = UINT32_1BYTE((uint16_t)(componentControl->periodPnPtTemperature[0] * 10.0));
		cfgVisualComponents[o++] = UINT32_0BYTE((uint16_t)(componentControl->periodPnPtTemperature[0] * 10.0));
 		cfgVisualComponents[o++] = UINT32_1BYTE((uint16_t)(componentControl->periodPnPtTemperature[1] * 10.0));
		cfgVisualComponents[o++] = UINT32_0BYTE((uint16_t)(componentControl->periodPnPtTemperature[1] * 10.0));
 		cfgVisualComponents[o++] = UINT32_1BYTE((uint16_t)(componentControl->periodPnPtTemperature[2] * 10.0));
		cfgVisualComponents[o++] = UINT32_0BYTE((uint16_t)(componentControl->periodPnPtTemperature[2] * 10.0));
 		cfgVisualComponents[o++] = UINT32_1BYTE((uint16_t)(componentControl->periodPnPtTemperature[3] * 10.0));
		cfgVisualComponents[o++] = UINT32_0BYTE((uint16_t)(componentControl->periodPnPtTemperature[3] * 10.0));
 		cfgVisualComponents[o++] = UINT32_1BYTE((uint16_t)(componentControl->periodSaTemperature[0] * 10.0));
		cfgVisualComponents[o++] = UINT32_0BYTE((uint16_t)(componentControl->periodSaTemperature[0] * 10.0));
 		cfgVisualComponents[o++] = UINT32_1BYTE((uint16_t)(componentControl->periodSaTemperature[1] * 10.0));
		cfgVisualComponents[o++] = UINT32_0BYTE((uint16_t)(componentControl->periodSaTemperature[1] * 10.0));
 		cfgVisualComponents[o++] = UINT32_1BYTE((uint16_t)(componentControl->periodSaTemperature[2] * 10.0));
		cfgVisualComponents[o++] = UINT32_0BYTE((uint16_t)(componentControl->periodSaTemperature[2] * 10.0));
 		cfgVisualComponents[o++] = UINT32_1BYTE((uint16_t)(componentControl->periodSaTemperature[3] * 10.0));
		cfgVisualComponents[o++] = UINT32_0BYTE((uint16_t)(componentControl->periodSaTemperature[3] * 10.0));
 		cfgVisualComponents[o++] = UINT32_1BYTE((uint16_t)(componentControl->periodSuTemperature[0] * 10.0));
		cfgVisualComponents[o++] = UINT32_0BYTE((uint16_t)(componentControl->periodSuTemperature[0] * 10.0));
 		cfgVisualComponents[o++] = UINT32_1BYTE((uint16_t)(componentControl->periodSuTemperature[1] * 10.0));
		cfgVisualComponents[o++] = UINT32_0BYTE((uint16_t)(componentControl->periodSuTemperature[1] * 10.0));
 		cfgVisualComponents[o++] = UINT32_1BYTE((uint16_t)(componentControl->periodSuTemperature[2] * 10.0));
		cfgVisualComponents[o++] = UINT32_0BYTE((uint16_t)(componentControl->periodSuTemperature[2] * 10.0));
 		cfgVisualComponents[o++] = UINT32_1BYTE((uint16_t)(componentControl->periodSuTemperature[3] * 10.0));
		cfgVisualComponents[o++] = UINT32_0BYTE((uint16_t)(componentControl->periodSuTemperature[3] * 10.0));
		cfgVisualComponents[o++] = UINT32_1BYTE((uint16_t)(componentControl->maxFloorTemperature * 10.0));
		cfgVisualComponents[o++] = UINT32_0BYTE((uint16_t)(componentControl->maxFloorTemperature * 10.0));
		cfgVisualComponents[o++] = UINT32_1BYTE((uint16_t)(componentControl->hysteresisTemperature * 100.0));
		cfgVisualComponents[o++] = UINT32_0BYTE((uint16_t)(componentControl->hysteresisTemperature * 100.0));
	}
}

bool VisualComponent_LoadVisualComponentsFromFlash(struct FlashConfiguration *flashCfg)
{
	if (LogLevel >= 1)
		PrintFlashMemory("load", CONFIGURATION_VISUAL_COMPONENTS_FLASH_ADDRESS, 128);

	if (flashCfg->configurationSize == 0)
		return true;

	uint16_t o = 0;
	if (cfgVisualComponents[o++] != flashCfg->itemStoredSignature)
		return false;

	uint16_t size = cfgVisualComponents[o++] << 8;
	size |= cfgVisualComponents[o++];
	if (size != flashCfg->configurationSize)
		return false;

	uint16_t f;
	struct HeatingVisualComponent *component;
	struct HeatingVisualComponentControl *componentControl;
	for (uint16_t i = 0; i < heatingDevicesComponentsCount; i++)
	{
		component = &heatingDevicesComponents[i];
		componentControl = component->heatingVisualComponentControl;
		componentControl->mode = cfgVisualComponents[o++];
		componentControl->periodsPnPtCount = cfgVisualComponents[o++];
		componentControl->periodsSaCount = cfgVisualComponents[o++];
		componentControl->periodsSuCount = cfgVisualComponents[o++];
		componentControl->periodPnPtFrom[0] = cfgVisualComponents[o++];
		componentControl->periodPnPtFrom[1] = cfgVisualComponents[o++];
		componentControl->periodPnPtFrom[2] = cfgVisualComponents[o++];
		componentControl->periodPnPtFrom[3] = cfgVisualComponents[o++];
		componentControl->periodSaFrom[0] = cfgVisualComponents[o++];
		componentControl->periodSaFrom[1] = cfgVisualComponents[o++];
		componentControl->periodSaFrom[2] = cfgVisualComponents[o++];
		componentControl->periodSaFrom[3] = cfgVisualComponents[o++];
		componentControl->periodSuFrom[0] = cfgVisualComponents[o++];
		componentControl->periodSuFrom[1] = cfgVisualComponents[o++];
		componentControl->periodSuFrom[2] = cfgVisualComponents[o++];
		componentControl->periodSuFrom[3] = cfgVisualComponents[o++];
		f = cfgVisualComponents[o++] << 8;
		f |= cfgVisualComponents[o++];
		componentControl->manualTemperature = f / 10.0;
		f = cfgVisualComponents[o++] << 8;
		f |= cfgVisualComponents[o++];
		componentControl->periodPnPtTemperature[0] = f / 10.0;
		f = cfgVisualComponents[o++] << 8;
		f |= cfgVisualComponents[o++];
		componentControl->periodPnPtTemperature[1] = f / 10.0;
		f = cfgVisualComponents[o++] << 8;
		f |= cfgVisualComponents[o++];
		componentControl->periodPnPtTemperature[2] = f / 10.0;
		f = cfgVisualComponents[o++] << 8;
		f |= cfgVisualComponents[o++];
		componentControl->periodPnPtTemperature[3] = f / 10.0;
		f = cfgVisualComponents[o++] << 8;
		f |= cfgVisualComponents[o++];
		componentControl->periodSaTemperature[0] = f / 10.0;
		f = cfgVisualComponents[o++] << 8;
		f |= cfgVisualComponents[o++];
		componentControl->periodSaTemperature[1] = f / 10.0;
		f = cfgVisualComponents[o++] << 8;
		f |= cfgVisualComponents[o++];
		componentControl->periodSaTemperature[2] = f / 10.0;
		f = cfgVisualComponents[o++] << 8;
		f |= cfgVisualComponents[o++];
		componentControl->periodSaTemperature[3] = f / 10.0;
		f = cfgVisualComponents[o++] << 8;
		f |= cfgVisualComponents[o++];
		componentControl->periodSuTemperature[0] = f / 10.0;
		f = cfgVisualComponents[o++] << 8;
		f |= cfgVisualComponents[o++];
		componentControl->periodSuTemperature[1] = f / 10.0;
		f = cfgVisualComponents[o++] << 8;
		f |= cfgVisualComponents[o++];
		componentControl->periodSuTemperature[2] = f / 10.0;
		f = cfgVisualComponents[o++] << 8;
		f |= cfgVisualComponents[o++];
		componentControl->periodSuTemperature[3] = f / 10.0;
		f = cfgVisualComponents[o++] << 8;
		f |= cfgVisualComponents[o++];
		componentControl->maxFloorTemperature = f / 10.0;
		f = cfgVisualComponents[o++] << 8;
		f |= cfgVisualComponents[o++];
		componentControl->hysteresisTemperature = f / 100.0;
	}

	if (LogLevel >= 1)
		LOGE(TAG, "load end ok o %d size %d", o, size);

	return o == size;
}

bool VisualComponent_WriteVisualComponentsToFlash(struct FlashConfiguration *flashCfg)
{
	VisualComponent_WriteVisualComponentsToConfiguration(flashCfg);
	WriteConfigurationToFlash(flashCfg);

	if (LogLevel >= 1)
	{
		LOGE(TAG, "Write cfgVisualComponents %x", (uint)cfgVisualComponents);
		PrintMemory(TAG, cfgVisualComponents, (uint)cfgVisualComponents, 64);
		PrintFlashMemory(TAG, CONFIGURATION_VISUAL_COMPONENTS_FLASH_ADDRESS, 128);
	}

	return true;
}

bool VisualComponent_GetHeatingVisualComponentFromAddress(uint32_t address, uint8_t segment,
		struct HeatingVisualComponent **heating)
{
	uint16_t i = 0;
	struct HeatingVisualComponent *heating_;
	while (i < heatingDevicesComponentsCount)
	{
		heating_ = &heatingDevicesComponents[i++];
		if (heating_->deviceItem == address && heating_->deviceSegment == segment)
		{
			*heating = heating_;
			return true;
		}
	}
	return false;
}

bool VisualComponent_GetHeatingVisualComponentControlFromAddress(uint32_t address, uint8_t segment,
		struct HeatingVisualComponentControl **heatingControl)
{
	uint16_t i = 0;
	struct HeatingVisualComponent *heatingComponent;
	*heatingControl = NULL;
	while (i < heatingDevicesComponentsCount)
	{
		heatingComponent = &heatingDevicesComponents[i++];
		if (heatingComponent->deviceItem == address && heatingComponent->deviceSegment == segment)
		{
			*heatingControl = heatingComponent->heatingVisualComponentControl;
			return true;
		}
	}
	return false;
}

void VisualComponent_PrintHeatingVisualComponents(void)
{
	for (uint16_t i = 0; i < heatingDevicesComponentsCount; i++)
	{
		struct HeatingVisualComponent *component;
		component = &heatingDevicesComponents[i];
		VisualComponent_PrintHeatingVisualComponent(component);
	}
}

void VisualComponent_PrintHeatingVisualComponent(struct HeatingVisualComponent *component)
{
	LOGI(TAG, "HeatingVisualComponent:");
	LOGI(TAG, "  address: 0x%.8x", component->deviceItem);
	LOGI(TAG, "  segment: %d", component->deviceSegment);

	struct HeatingVisualComponentSubItem *subItem;
	subItem = component->subComponents;
	for (int32_t s = 0; s < component->subComponentsCount; s++)
	{
		VisualComponent_PrintHeatingVisualComponentSubItem(subItem);
		subItem++;
	}
}

void VisualComponent_PrintHeatingVisualComponentSubItem(struct HeatingVisualComponentSubItem *subItem)
{
	LOGI(TAG, "  HeatingVisualComponentSubItem:");
	LOGI(TAG, "    address: 0x%.8x", subItem->deviceItem);
	LOGI(TAG, "    segment: %d", subItem->deviceSegment);
}

void VisualComponent_PrintHeatingVisualComponentControls(void)
{
	struct HeatingVisualComponent *component;
	struct HeatingVisualComponentControl *componentControl;
	for (uint16_t i = 0; i < heatingDevicesComponentsCount; i++)
	{
		component = &heatingDevicesComponents[i];
		componentControl = component->heatingVisualComponentControl;
		VisualComponent_PrintHeatingVisualComponentControl(component->deviceItem,
				component->deviceSegment, componentControl);
	}
}

void VisualComponent_PrintHeatingVisualComponentControl(uint32_t address, uint8_t segment,
		struct HeatingVisualComponentControl *componentControl)
{
	LOGI(TAG, "HeatingVisualComponentControl:");
	LOGI(TAG, "  address: 0x%.8x", address);
	LOGI(TAG, "  segment: %d", segment);
	if (componentControl->mode == HEATING_VISUAL_COMPONENT_MODE_OFF)
		LOGI(TAG, "  mode: off");
	else if (componentControl->mode == HEATING_VISUAL_COMPONENT_MODE_AUTO)
		LOGI(TAG, "  mode: auto");
	else if (componentControl->mode == HEATING_VISUAL_COMPONENT_MODE_MANUAL)
		LOGI(TAG, "  mode: manual");
	else
		LOGI(TAG, "  mode: undefined");

	if (componentControl->periodsPnPtCount == 1)
		LOGI(TAG, "  periodPnPt: %.2f C", componentControl->periodPnPtTemperature[0]);
	else
		LOGI(TAG, "  period1PnPtFrom: %d:%02d - %.2f C",
				VisualComponent_TimeByteToHour(componentControl->periodPnPtFrom[0]),
				VisualComponent_TimeByteToMinute(componentControl->periodPnPtFrom[0]),
				componentControl->periodPnPtTemperature[0]);
	if (componentControl->periodsPnPtCount >= 2)
		LOGI(TAG, "  period2PnPtFrom: %d:%02d - %.2f C",
				VisualComponent_TimeByteToHour(componentControl->periodPnPtFrom[1]),
				VisualComponent_TimeByteToMinute(componentControl->periodPnPtFrom[1]),
			componentControl->periodPnPtTemperature[1]);
	if (componentControl->periodsPnPtCount >= 3)
		LOGI(TAG, "  period3PnPtFrom: %d:%02d - %.2f C",
				VisualComponent_TimeByteToHour(componentControl->periodPnPtFrom[2]),
				VisualComponent_TimeByteToMinute(componentControl->periodPnPtFrom[2]),
			componentControl->periodPnPtTemperature[2]);
	if (componentControl->periodsPnPtCount >= 4)
		LOGI(TAG, "  period4PnPtFrom: %d:%02d - %.2f C",
				VisualComponent_TimeByteToHour(componentControl->periodPnPtFrom[3]),
				VisualComponent_TimeByteToMinute(componentControl->periodPnPtFrom[3]),
			componentControl->periodPnPtTemperature[3]);

	if (componentControl->periodsSaCount == 1)
		LOGI(TAG, "  periodSa: %.2f C", componentControl->periodSaTemperature[0]);
	else
		LOGI(TAG, "  period1SaFrom: %d:%02d - %.2f C",
				VisualComponent_TimeByteToHour(componentControl->periodSaFrom[0]),
				VisualComponent_TimeByteToMinute(componentControl->periodSaFrom[0]),
				componentControl->periodSaTemperature[0]);
	if (componentControl->periodsSaCount >= 2)
		LOGI(TAG, "  period2SaFrom: %d:%02d - %.2f C",
				VisualComponent_TimeByteToHour(componentControl->periodSaFrom[1]),
				VisualComponent_TimeByteToMinute(componentControl->periodSaFrom[1]),
			componentControl->periodSaTemperature[1]);
	if (componentControl->periodsSaCount >= 3)
		LOGI(TAG, "  period3SaFrom: %d:%02d - %.2f C",
				VisualComponent_TimeByteToHour(componentControl->periodSaFrom[2]),
				VisualComponent_TimeByteToMinute(componentControl->periodSaFrom[2]),
			componentControl->periodSaTemperature[2]);
	if (componentControl->periodsSaCount >= 4)
		LOGI(TAG, "  period4SaFrom: %d:%02d - %.2f C",
				VisualComponent_TimeByteToHour(componentControl->periodSaFrom[3]),
				VisualComponent_TimeByteToMinute(componentControl->periodSaFrom[3]),
			componentControl->periodSaTemperature[3]);

	if (componentControl->periodsSuCount == 1)
		LOGI(TAG, "  periodSu: %.2f C", componentControl->periodSuTemperature[0]);
	else
		LOGI(TAG, "  period1SuFrom: %d:%02d - %.2f C",
				VisualComponent_TimeByteToHour(componentControl->periodSuFrom[0]),
				VisualComponent_TimeByteToMinute(componentControl->periodSuFrom[0]),
				componentControl->periodSuTemperature[0]);
	if (componentControl->periodsSuCount >= 2)
		LOGI(TAG, "  period2SuFrom: %d:%02d - %.2f C",
				VisualComponent_TimeByteToHour(componentControl->periodSuFrom[1]),
				VisualComponent_TimeByteToMinute(componentControl->periodSuFrom[1]),
			componentControl->periodSuTemperature[1]);
	if (componentControl->periodsSuCount >= 3)
		LOGI(TAG, "  period3SuFrom: %d:%02d - %.2f C",
				VisualComponent_TimeByteToHour(componentControl->periodSuFrom[2]),
				VisualComponent_TimeByteToMinute(componentControl->periodSuFrom[2]),
			componentControl->periodSuTemperature[2]);
	if (componentControl->periodsSuCount >= 4)
		LOGI(TAG, "  period4SuFrom: %d:%02d - %.2f C",
				VisualComponent_TimeByteToHour(componentControl->periodSuFrom[3]),
				VisualComponent_TimeByteToMinute(componentControl->periodSuFrom[3]),
			componentControl->periodSuTemperature[3]);

	LOGI(TAG, "  maxFloorTemperature: %.2f C", componentControl->maxFloorTemperature);
	LOGI(TAG, "  hysteresisTemperature: +- %.2f C", componentControl->hysteresisTemperature);
}

#endif
