#include "../../include/main.h"
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "adc.h"

// static const char *TAG = "ADC";

#define DEFAULT_VREF    1100        // Use adc_vref_to_gpio(unit, gpio) to obtain a better estimate

static esp_adc_cal_characteristics_t *adcChars;

uint16_t vin = 0;
static uint32_t vinSum = 0;
static uint16_t vinCount = 0;
static uint32_t lastMeasure = 0;
static esp_adc_cal_value_t calType;

void Adc_Initialize(void)
{
	// eFuse Two Point burned into eFuse: NOT supported
	// esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK

	// eFuse Vref burned into eFuse: Supported
	// esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK

		// Configure ADC
	if (VIN_ADC_UNIT == ADC_UNIT_1)
	{
		adc1_config_width(VIN_ADC_WIDTH);
		adc1_config_channel_atten((adc_channel_t)VIN_ADC_CHANNEL, VIN_ADC_ATTEN);
	}
	else // ADC_UNIT_2
		adc2_config_channel_atten((adc_channel_t)VIN_ADC_CHANNEL, VIN_ADC_ATTEN);

		// Characterize ADC
	adcChars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
	calType = esp_adc_cal_characterize(VIN_ADC_UNIT, VIN_ADC_ATTEN, VIN_ADC_WIDTH, DEFAULT_VREF, adcChars);
	// ESP_ADC_CAL_VAL_EFUSE_TP - Characterized using Two Point Value
	// ESP_ADC_CAL_VAL_EFUSE_VREF - Characterized using eFuse Vref
	// ESP_ADC_CAL_VAL_DEFAULT_VREF - Characterized using Default Vref

// LOGE(TAG, "cal type = %d", calType);
// LOGI(TAG, "adcChars: %u", (uint)adcChars);
// if (adcChars != NULL)
// {
// LOGI(TAG, "  adc_num: %d", adcChars->adc_num);
// LOGI(TAG, "  atten: %d", adcChars->atten);
// LOGI(TAG, "  bit_width: %d", adcChars->bit_width);
// LOGI(TAG, "  coeff_a: %d", adcChars->coeff_a);
// LOGI(TAG, "  coeff_b: %d", adcChars->coeff_b);
// LOGI(TAG, "  high_curve: %u", (uint)adcChars->high_curve);
// LOGI(TAG, "  low_curve: %u", (uint)adcChars->low_curve);
// LOGI(TAG, "  version: %d", adcChars->version);
// LOGI(TAG, "  vref: %d", adcChars->vref);
// }

	// VREF 1100mV
	// ADC_ATTEN_DB_11 - * 3.55
	// ADC_ATTEN_DB_11 (effective: 150mV - 2450mv)
	// 10k/(10k+36k)=0,2174  2,6088V -> 12V
}

uint16_t Adc_Read(void)
{
	int raw;
	if (VIN_ADC_UNIT == ADC_UNIT_1)
		raw = adc1_get_raw((adc1_channel_t)VIN_ADC_CHANNEL);	// 0-4095
	else // ADC_UNIT_2
	{
		if (adc2_get_raw((adc2_channel_t)VIN_ADC_CHANNEL, VIN_ADC_WIDTH, &raw) != STATUS_OK)	// 0-4095
			return UINT16_MAX;
	}

	if (adcChars == NULL)
		return 0;
	uint32_t voltage = esp_adc_cal_raw_to_voltage(raw, adcChars);
	uint16_t voltage2 = voltage * 12000 / 2609;  // Vin before diode SK16 - 280mV
// LOGI(TAG, "Raw: %d\tVoltage: %dmV\tVoltage2: %dmV", raw, voltage, voltage2);
	return voltage2;
}

void Adc_Loop(void)
{
	uint32_t ms = GET_MS();
	if (ms - lastMeasure >= 20)
	{
		lastMeasure = ms;
		uint16_t value = Adc_Read();
		if (value == UINT16_MAX)
			return;

		vinSum += value;
		if (++vinCount == 25)		// 20ms * 25 = 500ms
		{
			vin = vinSum / vinCount;
			vinSum = 0;
			vinCount = 0;
		}
	}
}

/*
	https://docs.espressif.com/projects/esp-idf/en/v4.2-beta1/esp32/api-reference/peripherals/adc.html
	https://esp32tutorials.com/esp32-adc-esp-idf/
	https://github.com/espressif/esp-idf/blob/4143a3f572f6060258ffac91f110a095a8478966/examples/peripherals/adc/single_read/adc/main/adc1_example_main.c
*/
