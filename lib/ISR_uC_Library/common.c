#include "../../include/main.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#ifdef ESP32
	#include "esp_log.h"
	#include "esp_err.h"
	#include "esp_flash.h"
#endif
#ifdef STM32F0
	#include "stm32f0xx_hal.h"
	#include "stm32f0xx_init.h"
#endif
#ifdef STM32F4
	#include "stm32f4xx_hal.h"
	#include "stm32f4xx_init.h"
#endif
#include "common.h"

uint8_t BcdToByte(uint8_t a)
{
	return ((a & 0xf0) >> 4) * 10 + (a & 0x0f);
}

uint8_t ByteToBcd(uint8_t a)
{
	uint8_t a1 = a / 100;
	uint8_t a2 = a / 10 - a1 * 10;
	uint8_t a3 = a - a1 * 100 - a2 * 10;
	return (a2 << 4) | a3;
}

void PrintMemory(const char* TAG, const uint8_t* buffer, uint32_t address, uint32_t size)
{
	char printHex[64] = { 0 };
	char printAscii[20] = { 0 };
	char printTmp[3 + 1];
	char printChar[2] = { 0 };
	for (uint32_t i = 0; i < size; i++)
	{
		if (i % 16 == 0)
		{
			printHex[0] = 0;
			printAscii[0] = 0;
			if (address + size < 0xffff)
				sprintf(printHex, "0x%04x:", (unsigned int)(address + i));
			else
				sprintf(printHex, "0x%08x:", (unsigned int)(address + i));
		}

		sprintf(printTmp, " %02x", (uint8_t)buffer[i]);
		strcat(printHex, printTmp);

		if ((uint8_t)buffer[i] >= 32 && (uint8_t)buffer[i] < 127)
			printChar[0] = buffer[i];
		else
			printChar[0] = '.';
		strcat(printAscii, printChar);

		if (i % 4 == 4 - 1)
		{
			printTmp[0] = ' ';
			printTmp[1] = 0;
			strcat(printHex, printTmp);
		}
		if (i % 16 == 16 - 1)
			LOGI(TAG, "%s|%s|", printHex, printAscii);
	}

	// uint32_t address = (0x1000 + i);
	// uint8_t data = (uint8_t)(*((volatile uint8_t*)address));
}

#ifdef ESP32
void PrintFlashMemory(const char* TAG, uint32_t address, uint32_t size)
{
	uint8_t buffer[size];
	if (esp_flash_read(esp_flash_default_chip, buffer, address, size) != ESP_OK)
	{
		LOGI(TAG, "Can't read memory at %08x size %x", address, size);
	}
	else
		PrintMemory(TAG, buffer, address, size);
}

bool CompareMemory(const char* TAG, uint32_t address, uint8_t item[], uint32_t size)
{
	uint8_t buffer[size];
	if (esp_flash_read(esp_flash_default_chip, buffer, address, size) != ESP_OK)
	{
		LOGI(TAG, "Can't read memory at %08x size %x", address, size);
		return false;
	}

	for (uint32_t i = 0; i < size; i++)
		if (item[i] != buffer[i])
			return false;
	return true;
}
#endif

#if defined(STM32F0) || defined(STM32F4)
uint16_t GetMiliseconds(void)
{
	return htim1.Instance->CNT * 1000 / htim1.Init.Period;
}

// Precise blocked delay - works in interrupt
void Delay(uint16_t miliseconds)
{
	uint16_t start = htim1.Instance->CNT * 1000 / htim1.Init.Period;
	uint16_t end = start + miliseconds;
	end %= 1000;
	if (end > start)
		while (htim1.Instance->CNT * 1000 / htim1.Init.Period < end) ;
	else
		while (htim1.Instance->CNT * 1000 / htim1.Init.Period >= start ||
				htim1.Instance->CNT * 1000 / htim1.Init.Period < end) ;
}

uint16_t CountDelay(uint16_t tickStart, uint16_t tickEnd)
{
	if (tickEnd >= tickStart)
		return tickEnd - tickStart;
	return 1000UL + tickEnd - tickStart;
}
#endif

#ifdef STM32F4
uint32_t usTicks = 0;

uint8_t DWT_Delay_Init(void)
{
		/// Disable TRC
	CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk; // ~0x01000000
		/// Enable TRC
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // 0x01000000
		/// Disable clock cycle counter
	DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk; // ~0x00000001
		/// Enable clock cycle counter
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; // 0x00000001
		/// Reset the clock cycle counter value
	DWT->CYCCNT = 0;

	usTicks = HAL_RCC_GetSysClockFreq() / 1000000;

		/// Check if clock cycle counter has started
	if (DWT->CYCCNT)
		return 0;
	else
		return 1;
}

uint32_t GetMicroseconds(void)
{
	return DWT->CYCCNT / usTicks;
}

// Precise blocked delay - works in interrupt
void Delay_us(uint32_t microseconds)
{
	uint32_t start = DWT->CYCCNT;
	uint32_t end = start + microseconds * usTicks;
	if (end > start)
		while (DWT->CYCCNT < end) ;
	else
		while (DWT->CYCCNT >= start || DWT->CYCCNT < end) ;
}

uint32_t CountMicroseconds(uint32_t tickStart, uint32_t tickEnd)
{
	if (tickEnd >= tickStart)
		return tickEnd - tickStart;
	return 1UL + tickEnd - tickStart;
}
#endif
