#ifndef DEVICE_ITEM2_H_
#define DEVICE_ITEM2_H_

#include "../../include/main.h"
#if !defined(BOOTLOADER) && defined(ISR_CU)
#include <stdio.h>
#include "configuration_cu_control.h"

#define HEATING_VISUAL_COMPONENT_MODE_OFF    0
#define HEATING_VISUAL_COMPONENT_MODE_AUTO   1
#define HEATING_VISUAL_COMPONENT_MODE_MANUAL 2

struct HeatingVisualComponent
{
	uint32_t deviceItem;
	uint8_t deviceSegment;
	uint16_t subComponentsCount;
	void *subComponents;							// HeatingVisualComponentSubItem
	struct HeatingVisualComponentControl *heatingVisualComponentControl;
};

struct HeatingVisualComponentSubItem
{
	uint32_t deviceItem;
	uint8_t deviceSegment;
};

struct HeatingVisualComponentControl
{
	uint8_t mode;									// HEATING_VISUAL_COMPONENT_MODE_OFF, ...
	uint8_t periodsPnPtCount;
	uint8_t periodsSaCount;
	uint8_t periodsSuCount;
	uint8_t periodPnPtFrom[4];
	uint8_t periodSaFrom[4];
	uint8_t periodSuFrom[4];
	float manualTemperature;
	float periodPnPtTemperature[4];
	float periodSaTemperature[4];
	float periodSuTemperature[4];
	float maxFloorTemperature;
	float hysteresisTemperature;
};

extern struct HeatingVisualComponent *heatingDevicesComponents;
extern uint16_t heatingDevicesComponentsCount;

extern void VisualComponent_Initialize(void);
extern struct HeatingVisualComponent* VisualComponent_AddItem(uint16_t componentIndex,
		uint32_t address, uint8_t deviceSegment, uint16_t subItemsCount);
extern void VisualComponent_AddSubItem(struct HeatingVisualComponent *component,
		uint16_t subItemIndex, uint32_t address, uint8_t deviceSegment);
extern void VisualComponent_InitializeItemControl(struct HeatingVisualComponentControl *componentControl);
extern void VisualComponent_InitializeConfigurationVisualComponents(struct FlashConfiguration *flashCfg,
		uint16_t *count, uint16_t *size);
extern void VisualComponent_WriteVisualComponentsToConfiguration(struct FlashConfiguration *flashCfg);
extern bool VisualComponent_LoadVisualComponentsFromFlash(struct FlashConfiguration *flashCfg);
extern bool VisualComponent_WriteVisualComponentsToFlash(struct FlashConfiguration *flashCfg);
extern bool VisualComponent_GetHeatingVisualComponentFromAddress(uint32_t address, uint8_t segment,
		struct HeatingVisualComponent **heating);
extern bool VisualComponent_GetHeatingVisualComponentControlFromAddress(uint32_t address, uint8_t segment,
		struct HeatingVisualComponentControl **heatingControl);
extern void VisualComponent_PrintHeatingVisualComponents(void);
extern void VisualComponent_PrintHeatingVisualComponent(struct HeatingVisualComponent *component);
extern void VisualComponent_PrintHeatingVisualComponentSubItem(struct HeatingVisualComponentSubItem *subItem);
extern void VisualComponent_PrintHeatingVisualComponentControls(void);
extern void VisualComponent_PrintHeatingVisualComponentControl(uint32_t address, uint8_t segment,
		struct HeatingVisualComponentControl *componentControl);

#endif

#endif // DEVICE_ITEM2_H_
