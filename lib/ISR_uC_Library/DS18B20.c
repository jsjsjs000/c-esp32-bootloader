/*
	https://www.youtube.com/watch?v=Ozx6dmGtV5A
	https://vindicator.pl/sites/default/files/projekty/1wire.pdf
	http://castor.am.gdynia.pl/~dorra/pliki/Magistrala%201-wire.pdf
	https://ep.com.pl/kursy/tutoriale/10219-32-bity-jak-najprosciej.-stm32f0-nieblokujaca-obsluga-interfejsu-1-wire.-cz.-7
	https://community.st.com/s/question/0D50X00009XkYyD/1wire-library-for-stm32-in-c
	https://stackoverflow.com/questions/51752284/how-to-calculate-crc8-in-c
	https://crccalc.com
	https://en.wikipedia.org/wiki/1-Wire
	https://pl.wikipedia.org/wiki/1-Wire
	https://botland.com.pl/pl/czujniki-temperatury/165-czujnik-temperatury-ds18b20-cyfrowy-1-wire-tht.html
	- https://stm32f4-discovery.net/2015/07/hal-library-05-onewire-for-stm32fxxx

	temperature format: 0x00000000'0000,0000 Celsius degres - div by 4
		0x7fff - error

	implement:
		DS18B20_Tick(); - every 20us

		initialize:
			for (uint8_t i = 0; i < sizeof(DS18B20Temperatures) / sizeof(int16_t); i++)
				DS18B20Temperatures[i] = 0x7fff;

			STM32:
				HAL_GPIO_WritePin(GPIOA, TEMP1_Pin|TEMP2_Pin|TEMP3_Pin|TEMP4_Pin, GPIO_PIN_SET);
				GPIO_InitStruct.Pin = TEMP1_Pin|TEMP2_Pin|TEMP3_Pin|TEMP4_Pin;
				GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
				GPIO_InitStruct.Pull = GPIO_NOPULL;
				GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
				HAL_GPIO_Init(TEMP1_GPIO_Port, &GPIO_InitStruct);
			ESP32:
				gpio_config_t io_conf;
				io_conf.intr_type = GPIO_INTR_DISABLE;
				io_conf.mode = GPIO_MODE_INPUT_OUTPUT_OD;
				io_conf.pin_bit_mask = 1ULL << DS18B20_GPIO_Port;
				io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
				io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
				gpio_config(&io_conf);
				gpio_set_level(TEST_GPIO1, 1);

		main.c:
			int16_t DS18B20Temperatures[4];

			void DS18B20_MeasureCallback(uint8_t number, int16_t temperature)
			{
				DS18B20Temperatures[number] = temperature;
			}

		main.h:
			extern int16_t DS18B20Temperatures[4];

			extern void DS18B20_MeasureCallback(uint8_t number, int16_t temperature);

	STM32 - set other interrupt priority as 1, TIM6 for DS18B20 as 0 (highest)
		HAL_NVIC_SetPriority(ADC1_IRQn, 1, 0);
		HAL_NVIC_SetPriority(TIM1_IRQn, 1, 0);
		HAL_NVIC_SetPriority(TIM2_IRQn, 1, 0);
		HAL_NVIC_SetPriority(TIM6_IRQn, 0, 0);
		HAL_NVIC_SetPriority(USART1_IRQn, 1, 0);
		HAL_NVIC_SetPriority(USART2_IRQn, 1, 0);
*/

#include <string.h>
#include "../../include/main.h"
#ifdef STM32F0
	#include "stm32f0xx_hal.h"
	#include "stm32f0xx_ll_gpio.h"
	#include "stm32f0xx_init.h"
#endif
#ifdef STM32F4
	#include "stm32f4xx_hal.h"
	#include "stm32f4xx_ll_gpio.h"
	#include "stm32f4xx_init.h"
#endif
#ifdef ESP32
	#include <stdio.h>
	#include <stdbool.h>
	#include "driver/gpio.h"
	#include "rom/ets_sys.h"
	#include "../ISR_ESP32_Library/timer.h"
#endif
#include "DS18B20.h"

#if defined(STM32F0) || defined(STM32F4)
	GPIO_InitTypeDef DS18B20_GPIO_InitStruct;
	GPIO_TypeDef *DS18B20_GPIO_Port;
#endif
#ifdef ESP32
	gpio_num_t DS18B20_GPIO_Port;
#endif

uint16_t DS18B20_Pin;
uint8_t DS18B20_Number;
uint8_t DS18B20_Command = DS18B20_COMMAND_NONE;
uint8_t DS18B20_SendByte = 0;
uint8_t DS18B20_ReadByte = 0;
uint32_t DS18B20_Bit = 0;
uint32_t DS18B20_SubBit = 0;
uint8_t DS18B20_ReceiveBuffer[DS18B20_RECEIVE_BUFFER_LENGTH];
volatile bool DS18B20_Active = false;

void DS18B20_MeasureCallback(uint8_t number, int16_t temperature) __attribute__((weak));

	/// CRC-8-Dallas/Maxim
uint8_t DS18B20_CalculateCrc(uint8_t *data, uint16_t length)
{
	uint8_t j, d, m;
	uint8_t crc = 0;
	for (uint16_t i = 0; i < length; i++)
	{
		d = *data++;
		for (j = 0; j < 8; j++)
		{
			m = (crc ^ d) & 0x01;
			crc >>= 1;
			if (m)
				crc ^= DS18B20_CRC_POLY;
			d >>= 1;
		}
	}
	return crc;
}

void DS18B20_SetAsInputLine(void)
{
#if defined(STM32F0) || defined(STM32F4)
	LL_GPIO_SetPinMode(DS18B20_GPIO_Port, DS18B20_Pin, LL_GPIO_MODE_INPUT);
#endif
}

void DS18B20_SetAsOutputLine(void)
{
#if defined(STM32F0) || defined(STM32F4)
	LL_GPIO_SetPinMode(DS18B20_GPIO_Port, DS18B20_Pin, LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinOutputType(DS18B20_GPIO_Port, DS18B20_Pin, LL_GPIO_OUTPUT_OPENDRAIN);
	LL_GPIO_SetOutputPin(DS18B20_GPIO_Port, DS18B20_Pin);
#endif
#ifdef ESP32
	DS18B20_SET_PIN();
#endif
}

void DS18B20_Init(bool readNextByte)
{
	if (readNextByte)
		DS18B20_ReadByte++;
	else
		DS18B20_ReadByte = 0;
	DS18B20_SubBit = 0;
	DS18B20_Bit = 0;
	DS18B20_SetAsOutputLine();
}

void DS18B20_NextCommand(void)
{
	DS18B20_Bit = 0;
	if (DS18B20_Command == DS18B20_COMMAND_RESET_1)
	{
		DS18B20_Init(false);
		DS18B20_SendByte = 0xcc;
		DS18B20_Command = DS18B20_COMMAND_WRITE_0xCC_1;
	}
	else if (DS18B20_Command == DS18B20_COMMAND_WRITE_0xCC_1)
	{
		DS18B20_Init(false);
		DS18B20_SendByte = 0x44;
		DS18B20_Command = DS18B20_COMMAND_WRITE_0x44;
	}
	else if (DS18B20_Command == DS18B20_COMMAND_WRITE_0x44)
	{
		DS18B20_Init(false);
		DS18B20_Command = DS18B20_COMMAND_NONE;
#ifdef ESP32
		StopTimer1();
#endif
	}

	else if (DS18B20_Command == DS18B20_COMMAND_RESET_2)
	{
		DS18B20_Init(false);
		DS18B20_SendByte = 0xcc;
		DS18B20_Command = DS18B20_COMMAND_WRITE_0xCC_2;
	}
	else if (DS18B20_Command == DS18B20_COMMAND_WRITE_0xCC_2)
	{
		DS18B20_Init(false);
		DS18B20_SendByte = 0xbe;
		DS18B20_Command = DS18B20_COMMAND_WRITE_0xBE;
	}
	else if (DS18B20_Command == DS18B20_COMMAND_WRITE_0xBE)
	{
		DS18B20_Init(false);
		DS18B20_ReadByte = 0;
		DS18B20_Command = DS18B20_COMMAND_READ;
	}
	else if (DS18B20_Command == DS18B20_COMMAND_READ)
	{
		DS18B20_Init(true);
		if (DS18B20_ReadByte >= 9)
		{
			DS18B20_Command = DS18B20_COMMAND_NONE;
#ifdef ESP32
			StopTimer1();
#endif

			uint8_t crc = DS18B20_CalculateCrc(DS18B20_ReceiveBuffer, 8);
			bool ok = false;
			for (uint8_t i = 0; i < 9; i++)
				if (DS18B20_ReceiveBuffer[i] != 0)
					ok = true;
			if (crc == DS18B20_ReceiveBuffer[8] && ok)
			{
				int16_t temperature = (DS18B20_ReceiveBuffer[1] << 8) | DS18B20_ReceiveBuffer[0];
				DS18B20_MeasureCallback(DS18B20_Number, temperature);
			}
			else
				DS18B20_MeasureCallback(DS18B20_Number, 0x7fff);
			DS18B20_Command = DS18B20_COMMAND_NONE;
		}
	}
}

/// every 20us
void DS18B20_Tick(void)
{
	if (DS18B20_Command == DS18B20_COMMAND_NONE)
		return;

	if (DS18B20_Command == DS18B20_COMMAND_RESET_1 ||
			DS18B20_Command == DS18B20_COMMAND_RESET_2)
	{
		if (DS18B20_SubBit == 0)
			DS18B20_RESET_PIN();
		else if (DS18B20_SubBit == 540 / DS18B20_TICK_US) /// 540us
			DS18B20_SET_PIN();
		else if (DS18B20_SubBit == 600 / DS18B20_TICK_US) /// 600us
		{
			DS18B20_SetAsInputLine();
			if (DS18B20_READ_PIN())
			{
					/// no presence signal from device
				DS18B20_Command = DS18B20_COMMAND_NONE;
#ifdef ESP32
				StopTimer1();
#endif
				DS18B20_MeasureCallback(DS18B20_Number, 0x7fff);
				return;
			}
		}
		else if (DS18B20_SubBit == 1000 / DS18B20_TICK_US) /// 1000us
		{
			DS18B20_NextCommand();
			return;
		}

		DS18B20_SubBit++;
	}
	else if (DS18B20_Command == DS18B20_COMMAND_WRITE_0xCC_1 ||
			DS18B20_Command == DS18B20_COMMAND_WRITE_0x44 ||
			DS18B20_Command == DS18B20_COMMAND_WRITE_0xCC_2 ||
			DS18B20_Command == DS18B20_COMMAND_WRITE_0xBE)
	{
		uint8_t subBit = DS18B20_SubBit % DS18B20_BIT_DIVIDE;
		if (subBit == 0)
			DS18B20_RESET_PIN();
		else if (subBit == 1 && ((DS18B20_SendByte & (1 << DS18B20_Bit)) != 0))
			DS18B20_SET_PIN();
		else if ((subBit == DS18B20_BIT_DIVIDE - 1) && ((DS18B20_SendByte & (1 << DS18B20_Bit)) == 0))
			DS18B20_SET_PIN();
		if (subBit == DS18B20_BIT_DIVIDE - 1)
			DS18B20_Bit++;
		if (DS18B20_SubBit == 8 * DS18B20_BIT_DIVIDE - 1)
		{
			DS18B20_NextCommand();
			return;
		}

		DS18B20_SubBit++;
	}
	else if (DS18B20_Command == DS18B20_COMMAND_READ)
	{
		uint8_t subBit = DS18B20_SubBit % DS18B20_BIT_DIVIDE;
		if (subBit == 0)
		{
			DS18B20_SetAsOutputLine();
			DS18B20_RESET_PIN();
		}
		else if (subBit == 1)
		{
			DS18B20_SET_PIN();

			DS18B20_SetAsInputLine();
			DS18B20_DELAY();

			if (DS18B20_READ_PIN())
				DS18B20_ReceiveBuffer[DS18B20_ReadByte] |= 1 << DS18B20_Bit;
		}
		if (subBit == DS18B20_BIT_DIVIDE - 1)
			DS18B20_Bit++;
		if (DS18B20_SubBit == 8 * DS18B20_BIT_DIVIDE - 1)
		{
			DS18B20_NextCommand();
			return;
		}

		DS18B20_SubBit++;
	}
}

void DS18B20_SendNone(void)
{
	DS18B20_Init(false);
	memset(DS18B20_ReceiveBuffer, 0, DS18B20_RECEIVE_BUFFER_LENGTH);
	DS18B20_Command = DS18B20_COMMAND_NONE;
}

#if defined(STM32F0) || defined(STM32F4)
	void DS18B20_Start1(uint8_t number, GPIO_TypeDef *port, uint16_t pin)
	{
		DS18B20_Number = number;
		DS18B20_GPIO_Port = port;
		DS18B20_Pin = pin;
		DS18B20_GPIO_InitStruct.Pin = pin;
		DS18B20_SendNone();
		DS18B20_Command = DS18B20_COMMAND_RESET_1;
	}

		/// Send after 800 ms after DS18B20_Start1()
	void DS18B20_Start2(uint8_t number, GPIO_TypeDef *port, uint16_t pin)
	{
		DS18B20_Number = number;
		DS18B20_GPIO_Port = port;
		DS18B20_Pin = pin;
		DS18B20_GPIO_InitStruct.Pin = pin;
		DS18B20_SendNone();
		DS18B20_Command = DS18B20_COMMAND_RESET_2;
	}
#endif

#ifdef ESP32
	void DS18B20_Start1(uint8_t number, gpio_num_t port)
	{
		DS18B20_Number = number;
		DS18B20_GPIO_Port = port;
		DS18B20_SendNone();
		DS18B20_Command = DS18B20_COMMAND_RESET_1;
		StartTimer1();
	}

		/// Send after 800 ms after DS18B20_Start1()
	void DS18B20_Start2(uint8_t number, gpio_num_t port)
	{
		DS18B20_Number = number;
		DS18B20_GPIO_Port = port;
		DS18B20_SendNone();
		DS18B20_Command = DS18B20_COMMAND_RESET_2;
		StartTimer1();
	}
#endif
