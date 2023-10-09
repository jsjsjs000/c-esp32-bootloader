#include "../../include/main.h"
#include <time.h>
#include <sys/time.h>
#include "esp_log.h"
#include "esp_sntp.h"
#include "rtc.h"
#include "rtc_ds3231.h"

static const char *TAG = "RTC";
static const uint8_t LogLevel = 0; /// 0 - 1

bool RTC_isSetRtc = false;
bool RTC_isSetRtcFromSntp = false;

static void SntpCallback(struct timeval *tv)
{
	RTC_isSetRtcFromSntp = true;
	RTC_isSetRtc = true;

	time_t now;
	struct tm timeinfo;
	struct tm timeinfo2;
	time(&now);
	localtime_r(&now, &timeinfo);
	RTC_ConvertFromSystemTM(&timeinfo, &timeinfo2);
	RTC_DS3231_Write(&timeinfo2);			// $$ write only if time different > 3s

	char tis[32];
	RTC_GetCurrentDateTimeString(tis, sizeof(tis));
	if (LogLevel > 0)
		LOGI(TAG, "SNTP callback - current date/time is: %s", tis);
}

void RTC_ConvertToSystemTM(struct tm *timeinfo, struct tm *tiout)
{
	tiout->tm_year = timeinfo->tm_year - 1900;
	tiout->tm_mon = timeinfo->tm_mon - 1;
	tiout->tm_mday = timeinfo->tm_mday;
	tiout->tm_wday = timeinfo->tm_wday;
	tiout->tm_hour = timeinfo->tm_hour;
	tiout->tm_min = timeinfo->tm_min;
	tiout->tm_sec = timeinfo->tm_sec;
	tiout->tm_isdst = -1;
}

void RTC_ConvertFromSystemTM(struct tm *timeinfo, struct tm *tiout)
{
	tiout->tm_year = timeinfo->tm_year + 1900;
	tiout->tm_mon = timeinfo->tm_mon + 1;
	tiout->tm_mday = timeinfo->tm_mday;
	tiout->tm_wday = timeinfo->tm_wday;
	tiout->tm_hour = timeinfo->tm_hour;
	tiout->tm_min = timeinfo->tm_min;
	tiout->tm_sec = timeinfo->tm_sec;
	tiout->tm_isdst = -1;
}

void RTC_SetSystemTime(struct tm *timeinfo, uint32_t usec)
{
	struct tm tm2;
	RTC_ConvertToSystemTM(timeinfo, &tm2);

	time_t t = mktime(&tm2);
	struct timeval tv;
	tv.tv_sec = t;
	tv.tv_usec = usec;
	settimeofday(&tv, NULL);
}

	/// min 32 chars
void RTC_GetCurrentDateTimeString(char* s, uint8_t sLength)
{
	time_t now;
	struct tm timeinfo;
	time(&now);
	localtime_r(&now, &timeinfo);
		// https://www.cplusplus.com/reference/ctime/strftime
	strftime(s, sLength, "%Y-%m-%d %a %H:%M:%S %Z", &timeinfo);
}

uint32_t RTC_GetLinuxTimestamp(void)
{
	time_t now;
	time(&now);
	localtime(&now);
	return (uint32_t)now;
}

void RTC_Initialize(void)
{
		// https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
	setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
	tzset();
}

void RTC_InitializeSntp(void)
{
		// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system_time.html
	sntp_set_time_sync_notification_cb(&SntpCallback);
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_setservername(0, SNTP_SERVER_NAME);
	// sntp_set_sync_interval(15 * 1000);		/// default interval is 1 hour
	sntp_init();
}

/*
	SNTP default interval: 1h (sdkconfig.h: CONFIG_LWIP_SNTP_UPDATE_DELAY)

	sntp_restart();

	https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system_time.html#_CPPv419sntp_sync_time_cb_t
*/
