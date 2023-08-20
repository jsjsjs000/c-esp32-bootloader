#ifndef __COMMON_H
#define __COMMON_H

#include "../../include/main.h"
#ifdef STM32F0
	#include "stm32f0xx_hal.h"
#endif
#ifdef STM32F4
	#include "stm32f4xx_hal.h"
#endif

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define SET_ARRAY_BIT(ARRAY, ADDRESS)		(ARRAY[(ADDRESS) / 8] |= 1 << ((ADDRESS) % 8))
#define CLEAR_ARRAY_BIT(ARRAY, ADDRESS)	(ARRAY[(ADDRESS) / 8] &= ~(1 << ((ADDRESS) % 8)))
#define GET_ARRAY_BIT(ARRAY, ADDRESS)		((ARRAY[(ADDRESS) / 8] & (1 << ((ADDRESS) % 8))) != 0)
#define GET_ARRAY_2BITS(ARRAY, ADDRESS)	((ARRAY[(ADDRESS) / 4] & (1 << (2 * ((ADDRESS) % 4)))) != 0)
#define SHIFT_BIT(BIT, SHIFT)						((BIT & 0x01) << SHIFT)

#define UINT32_FROM_ARRAY_LE(ARRAY, OFFSET)  (ARRAY[OFFSET] | ARRAY[(OFFSET) + 1] << 8 | \
																							ARRAY[(OFFSET) + 2] << 16 | ARRAY[(OFFSET) + 3] << 24)
#define UINT32_FROM_ARRAY_BE(ARRAY, OFFSET)  (ARRAY[OFFSET] << 24 | ARRAY[(OFFSET) + 1] << 16 | \
																							ARRAY[(OFFSET) + 2] << 8 | ARRAY[(OFFSET) + 3])
#define UINT16_FROM_ARRAY_LE(ARRAY, OFFSET)  (ARRAY[OFFSET] | ARRAY[(OFFSET) + 1] << 8)
#define UINT16_FROM_ARRAY_BE(ARRAY, OFFSET)  (ARRAY[OFFSET] << 8 | ARRAY[(OFFSET) + 1])

#define UINT32_3BYTE(A)   (((A) >> 24) & 0xff)
#define UINT32_2BYTE(A)   (((A) >> 16) & 0xff)
#define UINT32_1BYTE(A)   (((A) >> 8) & 0xff)
#define UINT32_0BYTE(A)   ((A) & 0xff)

#define UINT32_FROM_BYTES(BYTE3, BYTE2, BYTE1, BYTE0) ((BYTE3) << 24 | (BYTE2) << 16 | (BYTE1) << 8 | BYTE0)
#define UINT16_FROM_BYTES(BYTE1, BYTE0) ((BYTE1) << 8 | BYTE0)

#define ALIGN_UP_TO_4(A)	(((A) % 4 == 0) ? A : (((A) / 4) + 1) * 4)

#ifdef ESP32
#define RANDOM_TO(MAX_VALUE) (esp_random() % (MAX_VALUE))
#define RANDOM_FROM_TO(MIN_VALUE, MAX_VALUE) ((esp_random() % (MAX_VALUE - MIN_VALUE - 1)) + MIN_VALUE)
#endif

extern uint8_t BcdToByte(uint8_t a);
extern uint8_t ByteToBcd(uint8_t a);

extern void PrintMemory(const char* TAG, const uint8_t* buffer, uint32_t address, uint32_t size);
extern void PrintFlashMemory(const char* TAG, uint32_t address, uint32_t size);
extern bool CompareMemory(const char* TAG, uint32_t address, uint8_t item[], uint32_t size);

#if defined(STM32F0) || defined(STM32F4)
extern uint16_t GetMiliseconds(void);
/// Precise blocked delay - works in interrupt
extern void Delay(uint16_t miliseconds);
extern uint16_t CountDelay(uint16_t tickStart, uint16_t tickEnd);
#endif

#ifdef STM32F4
	/// Initialize microseconds counter
	extern uint8_t DWT_Delay_Init(void);
	extern uint32_t GetMicroseconds(void);
	/// Precise blocked delay - works in interrupt
	extern void Delay_us(uint32_t microseconds);
	extern uint32_t CountMicroseconds(uint32_t tickStart, uint32_t tickEnd);
#endif

#endif /* __COMMON_H */
