#include "../../include/main.h"
#include <stdio.h>
#include <string.h>
#include <esp_log.h>
#include <time.h>
#include <driver/i2c.h>
#include "../ISR_uC_Library/common.h"
#ifdef STM32F0
	#include "stm32f0xx_hal.h"
	#include "stm32f0xx_init.h"
#endif
#ifdef STM32F4
	#include "stm32f4xx_hal.h"
	#include "stm32f4xx_init.h"
#endif
#include "rtc_ds3231.h"
#include "rtc.h"

const char *TAG_RTC_DS3231 = "RTC_DS3231";

const char WeekDays[8][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

bool RTC_DS3231_isSetRtcFromDS3231 = false;

StatusType RTC_DS3231_Initialize(void)
{
	i2c_config_t i2c_conf = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = RTC_I2C_MASTER_SDA_IO,
		.sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_io_num = RTC_I2C_MASTER_SCL_IO,
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master.clk_speed = RTC_I2C_MASTER_FREQ_HZ,
	};
	if (i2c_param_config(RTC_I2C_PORT_NUMBER, &i2c_conf) != STATUS_OK)
		return STATUS_ERROR;

	if (i2c_driver_install(RTC_I2C_PORT_NUMBER, I2C_MODE_MASTER, 0, 0, 0) != STATUS_OK)
		return STATUS_ERROR;

	// i2c_set_timeout(RTC_I2C_PORT_NUMBER, pdMS_TO_TICKS(RTC_DS3231_TIMEOUT) * 1000);

	return STATUS_OK;
}

void RTC_DS3231_SetTimeInfo(struct tm *ti, uint16_t y, uint8_t mo, uint8_t d, uint8_t wd,
		uint8_t h, uint8_t mi, uint8_t s)
{
	ti->tm_sec = s;
	ti->tm_min = mi;
	ti->tm_hour = h;
	ti->tm_wday = wd;
	ti->tm_mday = d;
	ti->tm_mon = mo;
	ti->tm_year = y;
}

	/// 1 - sunday, 7 - saturday
StatusType RTC_DS3231_Read(struct tm *ti)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (RTC_DS3231_ADDRESS << 1) | I2C_MASTER_WRITE, I2C_MASTER_ACK);
	i2c_master_write_byte(cmd, 0, I2C_MASTER_ACK);
	i2c_master_stop(cmd);
	if (i2c_master_cmd_begin(RTC_I2C_PORT_NUMBER, cmd, pdMS_TO_TICKS(RTC_DS3231_TIMEOUT)) != STATUS_OK)
	{
		i2c_cmd_link_delete(cmd);
		return STATUS_ERROR;
	}
	i2c_cmd_link_delete(cmd);

	uint8_t y, mo, d, wd, h, mi, s;
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (RTC_DS3231_ADDRESS << 1) | I2C_MASTER_READ, true);
	i2c_master_read_byte(cmd, &s, I2C_MASTER_ACK);
	i2c_master_read_byte(cmd, &mi, I2C_MASTER_ACK);
	i2c_master_read_byte(cmd, &h, I2C_MASTER_ACK);
	i2c_master_read_byte(cmd, &wd, I2C_MASTER_ACK);
	i2c_master_read_byte(cmd, &d, I2C_MASTER_ACK);
	i2c_master_read_byte(cmd, &mo, I2C_MASTER_ACK);
	i2c_master_read_byte(cmd, &y, I2C_MASTER_NACK); // last read must be NO_ACK
	i2c_master_stop(cmd);
	if (i2c_master_cmd_begin(RTC_I2C_PORT_NUMBER, cmd, pdMS_TO_TICKS(RTC_DS3231_TIMEOUT)) != STATUS_OK)
	{
		i2c_cmd_link_delete(cmd);
		return STATUS_ERROR;
	}
	i2c_cmd_link_delete(cmd);

	s = BcdToByte(s);
	mi = BcdToByte(mi);
	h = BcdToByte(h & 0x3f);
	d = BcdToByte(d);
	mo = BcdToByte(mo & 0x1f);
	uint16_t y2 = BcdToByte(y) + 2000;

	if (s > 60 || mi > 60 || h > 24 || d > 31 || mo > 12 || y2 > 2040)
		return STATUS_ERROR;

	ti->tm_sec = s;
	ti->tm_min = mi;
	ti->tm_hour = h;
	ti->tm_wday = wd;
	ti->tm_mday = d;
	ti->tm_mon = mo;
	ti->tm_year = y2;

	RTC_DS3231_isSetRtcFromDS3231 = true;
	RTC_isSetRtc = true;
	return STATUS_OK;
}

	/// 1 - sunday, 7 - saturday
StatusType RTC_DS3231_Write(struct tm *ti)
{
	if (ti->tm_sec > 60 || ti->tm_min > 60 || ti->tm_hour > 24 ||
			ti->tm_mday > 31 || ti->tm_mon > 12 || ti->tm_year < 2000 || ti->tm_year > 2040)
		return STATUS_ERROR;

	uint8_t data[7];
	data[0] = ByteToBcd(ti->tm_sec);
	data[1] = ByteToBcd(ti->tm_min);
	data[2] = ByteToBcd(ti->tm_hour);
	data[3] = ti->tm_wday;
	data[4] = ByteToBcd(ti->tm_mday);
	data[5] = ByteToBcd(ti->tm_mon);
	data[6] = ByteToBcd(ti->tm_year - 2000);

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (RTC_DS3231_ADDRESS << 1) | I2C_MASTER_WRITE, I2C_MASTER_ACK);
	i2c_master_write_byte(cmd, 0, I2C_MASTER_ACK);
	for (uint8_t i = 0; i < sizeof(data); i++)
		i2c_master_write_byte(cmd, data[i], I2C_MASTER_ACK);
	i2c_master_stop(cmd);
	if (i2c_master_cmd_begin(RTC_I2C_PORT_NUMBER, cmd, pdMS_TO_TICKS(RTC_DS3231_TIMEOUT)) != STATUS_OK)
	{
		i2c_cmd_link_delete(cmd);
		return STATUS_ERROR;
	}
	i2c_cmd_link_delete(cmd);

	return STATUS_OK;
}

void RTC_DS3231_GetDateString(struct tm *ti, char *s, uint8_t sLength)
{
	sprintf(s, "%04d-%02d-%02d", ti->tm_year, ti->tm_mon, ti->tm_mday);
}

void RTC_DS3231_GetTimeString(struct tm *ti, char *s, uint8_t sLength)
{
	sprintf(s, "%d:%02d:%02d", ti->tm_hour, ti->tm_min, ti->tm_sec);
}

void RTC_DS3231_GetDateTimeString(struct tm *ti, char *s, uint8_t sLength)
{
	sprintf(s, "%04d-%02d-%02d %s %d:%02d:%02d", ti->tm_year, ti->tm_mon, ti->tm_mday,
			WeekDays[ti->tm_wday], ti->tm_hour, ti->tm_min, ti->tm_sec);
}

/*
	https://github.com/pantaluna/esp32_ds3231_clock_using_lib
	https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2c.html
	https://datasheets.maximintegrated.com/en/ds/DS3231.pdf
	https://web.wpi.edu/Pubs/E-project/Available/E-project-010908-124414/unrestricted/DS3231-DS3231S.pdf
*/
