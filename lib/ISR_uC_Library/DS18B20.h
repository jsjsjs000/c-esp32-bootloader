#ifndef __DS18B20_H
#define __DS18B20_H

#ifdef STM32F0
	#include "stm32f0xx_hal.h"
#endif
#ifdef STM32F4
	#include "stm32f4xx_hal.h"
#endif
#ifdef ESP32
	#include "driver/gpio.h"
#endif

#define DS18B20_TICK_US								20
#define DS18B20_BIT_DIVIDE						4

#define DS18B20_COMMAND_NONE					0
#define DS18B20_COMMAND_RESET_1				1
#define DS18B20_COMMAND_WRITE_0xCC_1	2
#define DS18B20_COMMAND_WRITE_0x44		3
#define DS18B20_COMMAND_RESET_2				11
#define DS18B20_COMMAND_WRITE_0xCC_2	12
#define DS18B20_COMMAND_WRITE_0xBE		13
#define DS18B20_COMMAND_READ					14

#define DS18B20_RECEIVE_BUFFER_LENGTH	10

#define DS18B20_CRC_POLY 0x8c         /// X^8 + X^5 + X^4 + X^0

#if defined(STM32F0) || defined(STM32F4)
	//$$ #define DS18B20_READ_PIN()		(HAL_GPIO_ReadPin(DS18B20_GPIO_Port, DS18B20_Pin) != GPIO_PIN_RESET)
	#define DS18B20_READ_PIN()		(LL_GPIO_IsInputPinSet(DS18B20_GPIO_Port, DS18B20_Pin) != 0)
	#define DS18B20_RESET_PIN() 	HAL_GPIO_WritePin(DS18B20_GPIO_Port, DS18B20_Pin, GPIO_PIN_RESET)
	#define DS18B20_SET_PIN()			HAL_GPIO_WritePin(DS18B20_GPIO_Port, DS18B20_Pin, GPIO_PIN_SET)
	#define DS18B20_DELAY()			\
			__ASM volatile("nop");	\
			__ASM volatile("nop");	\
			__ASM volatile("nop");	\
			__ASM volatile("nop");

#endif
#ifdef ESP32
	#define DS18B20_READ_PIN()		(gpio_get_level(DS18B20_GPIO_Port) != 0)
	#define DS18B20_RESET_PIN() 	gpio_set_level(DS18B20_GPIO_Port, 0)
	#define DS18B20_SET_PIN()			gpio_set_level(DS18B20_GPIO_Port, 1)
	#define DS18B20_DELAY()				ets_delay_us(5)
#endif

extern void DS18B20_Tick(void);

#if defined(STM32F0) || defined(STM32F4)
	extern void DS18B20_Start1(uint8_t number, GPIO_TypeDef *port, uint16_t pin);
		/// Send after 800 ms after DS18B20_Start1()
	extern void DS18B20_Start2(uint8_t number, GPIO_TypeDef *port, uint16_t pin);
#endif
#ifdef ESP32
	extern void DS18B20_Start1(uint8_t number, gpio_num_t port);
	extern void DS18B20_Start2(uint8_t number, gpio_num_t port);
#endif

extern volatile bool DS18B20_Active;

#endif /* __DS18B20_H */
