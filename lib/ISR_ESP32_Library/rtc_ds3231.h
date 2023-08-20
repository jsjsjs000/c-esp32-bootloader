#ifndef __RTC_DS3231_H
#define __RTC_DS3231_H

#include <stdio.h>

#define RTC_I2C_PORT_NUMBER     0
#define RTC_I2C_MASTER_SDA_IO   5
#define RTC_I2C_MASTER_SCL_IO   4
#define RTC_I2C_MASTER_FREQ_HZ  100000

#define RTC_DS3231_ADDRESS 0x68
#define RTC_DS3231_TIMEOUT 10

extern bool RTC_DS3231_isSetRtcFromDS3231;

extern StatusType RTC_DS3231_Initialize(void);
extern void RTC_DS3231_SetTimeInfo(struct tm *ti, uint16_t y, uint8_t mo, uint8_t d, uint8_t wd,
		uint8_t h, uint8_t mi, uint8_t s);
extern StatusType RTC_DS3231_Read(struct tm *ti);
extern StatusType RTC_DS3231_Write(struct tm *ti);
extern void RTC_DS3231_GetDateString(struct tm *ti, char *s, uint8_t sLength);
extern void RTC_DS3231_GetTimeString(struct tm *ti, char *s, uint8_t sLength);
extern void RTC_DS3231_GetDateTimeString(struct tm *ti, char *s, uint8_t sLength);

#endif /* __RTC_DS3231_H */
