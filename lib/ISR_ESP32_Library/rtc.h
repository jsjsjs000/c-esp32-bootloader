#define SNTP_SERVER_NAME   "pool.ntp.org"

extern bool RTC_isSetRtc;
extern bool RTC_isSetRtcFromSntp;

extern void RTC_ConvertToSystemTM(struct tm *timeinfo, struct tm *tiout);
extern void RTC_ConvertFromSystemTM(struct tm *timeinfo, struct tm *tiout);
extern void RTC_SetSystemTime(struct tm *timeinfo, uint32_t usec);
extern void RTC_GetCurrentDateTimeString(char* s, uint8_t sLength);
extern uint32_t RTC_GetLinuxTimestamp(void);
extern void RTC_Initialize(void);
extern void RTC_InitializeSntp(void);
