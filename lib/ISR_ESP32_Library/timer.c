#include "../../include/main.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_types.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"
#include "../ISR_uC_Library/DS18B20.h"

// #define TIMER0_DIVIDER         16                                 // Hardware timer clock divider
// #define TIMER0_SCALE           (TIMER_BASE_CLK / TIMER0_DIVIDER)  // convert counter value to seconds
// #define TIMER0_INTERVAL0_SEC   0.001                              // 1ms - 1kHz
#define TIMER1_DIVIDER         16                                 // Hardware timer clock divider
#define TIMER1_SCALE           (TIMER_BASE_CLK / TIMER1_DIVIDER)  // convert counter value to seconds
#define TIMER1_INTERVAL0_SEC   0.00002                            // 20us - 50kHz

// Timers collide with flash erase

// bool g1;
// bool g2;

/*
 * Timer group0 ISR handler
 *
 * Note:
 * We don't call the timer API here because they are not declared with IRAM_ATTR.
 * If we're okay with the timer irq not being serviced while SPI flash cache is disabled,
 * we can allocate this interrupt without the ESP_INTR_FLAG_IRAM flag and use the normal API.
 */
void IRAM_ATTR timer0_group0_isr(void *param)
{
	// gpio_set_level(TEST_GPIO1, g1 ? 0 : 1);
	// g1 = !g1;

	miliseconds++;

		/* Retrieve the interrupt status and the counter value
			from the timer that reported the interrupt */
	// timer_intr_t timer_intr = timer_group_intr_get_in_isr(TIMER_GROUP_0);
	// uint64_t timer_counter_value = timer_group_get_counter_value_in_isr(TIMER_GROUP_0, TIMER_0);

	timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);
	// timer_counter_value += (uint64_t)(TIMER_INTERVAL0_SEC * TIMER_SCALE);
	// timer_group_set_alarm_value_in_isr(TIMER_GROUP_0, TIMER_0, timer_counter_value);

		/* After the alarm has been triggered
			we need enable it again, so it is triggered the next time */
	timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_0);
}

void IRAM_ATTR timer1_group0_isr(void *param)
{
	// gpio_set_level(TEMP2_GPIO_Port, g2 ? 0 : 1);
	// g2 = !g2;

#ifdef ISR_TEMP
	DS18B20_Tick();
#endif
		/* Retrieve the interrupt status and the counter value
			from the timer that reported the interrupt */
	// timer_intr_t timer_intr = timer_group_intr_get_in_isr(TIMER_GROUP_0);
	// uint64_t timer_counter_value = timer_group_get_counter_value_in_isr(TIMER_GROUP_0, TIMER_1);

	timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_1);
	// timer_counter_value += (uint64_t)(TIMER_INTERVAL0_SEC * TIMER_SCALE);
	// timer_group_set_alarm_value_in_isr(TIMER_GROUP_0, TIMER_1, timer_counter_value);

		/* After the alarm has been triggered
			we need enable it again, so it is triggered the next time */
	timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_1);
}

void InitializeTimer(void)
{
		/* TIMER_0 */
	// timer_config_t config0;
	// config0.divider = TIMER0_DIVIDER;
	// config0.counter_dir = TIMER_COUNT_UP;
	// config0.counter_en = TIMER_PAUSE;
	// config0.alarm_en = TIMER_ALARM_EN;
	// config0.intr_type = TIMER_INTR_LEVEL;
	// config0.auto_reload = TIMER_AUTORELOAD_EN;
	// timer_init(TIMER_GROUP_0, TIMER_0, &config0);

	// 	/* Timer's counter will initially start from value below.
	// 		Also, if auto_reload is set, this value will be automatically reload on alarm */
	// timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0x00000000ULL);

	// 	/* Configure the alarm value and the interrupt on alarm. */
	// timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, TIMER0_INTERVAL0_SEC * TIMER0_SCALE);
	// timer_enable_intr(TIMER_GROUP_0, TIMER_0);
	// timer_isr_register(TIMER_GROUP_0, TIMER_0, &timer0_group0_isr,
	// 		(void *)TIMER_0, ESP_INTR_FLAG_IRAM, NULL);

	// timer_start(TIMER_GROUP_0, TIMER_0);

		/* TIMER_1 */
	timer_config_t config1;
	config1.divider = TIMER1_DIVIDER;
	config1.counter_dir = TIMER_COUNT_UP;
	config1.counter_en = TIMER_PAUSE;
	config1.alarm_en = TIMER_ALARM_EN;
	config1.intr_type = TIMER_INTR_LEVEL;
	config1.auto_reload = TIMER_AUTORELOAD_EN;
	timer_init(TIMER_GROUP_0, TIMER_1, &config1);

		/* Timer's counter will initially start from value below.
			Also, if auto_reload is set, this value will be automatically reload on alarm */
	timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0x00000000ULL);

		/* Configure the alarm value and the interrupt on alarm. */
	timer_set_alarm_value(TIMER_GROUP_0, TIMER_1, TIMER1_INTERVAL0_SEC * TIMER1_SCALE);
	timer_enable_intr(TIMER_GROUP_0, TIMER_1);
	timer_isr_register(TIMER_GROUP_0, TIMER_1, &timer1_group0_isr,
			(void *)TIMER_1, ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL3, NULL);
	timer_pause(TIMER_GROUP_0, TIMER_1);
}

void StartTimer1(void)
{
	timer_start(TIMER_GROUP_0, TIMER_1);
	vTaskSuspendAll();

	DS18B20_Active = true;
	while (DS18B20_Active)
	{
		__asm__ __volatile__ ("nop");
	}

	timer_pause(TIMER_GROUP_0, TIMER_1);
	xTaskResumeAll();
}

void StopTimer1(void)
{
	DS18B20_Active = false;
}
