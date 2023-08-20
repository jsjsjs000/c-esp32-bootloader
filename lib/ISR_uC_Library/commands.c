#include "../../include/main.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "packets.h"
#include "rtc.h"

uint16_t Command_PrepareSynchronization(uint8_t *txBuffer, uint16_t txBufferLength)
{
	time_t now;
	struct tm timeinfo;
	struct tm timeinfo2;
	time(&now);
	localtime_r(&now, &timeinfo);
	RTC_ConvertFromSystemTM(&timeinfo, &timeinfo2);

	struct timeval tv;
	gettimeofday(&tv, NULL);
	uint16_t msec = tv.tv_usec / 1000;

	uint16_t j = PACKET_PRE_BYTES;
	txBuffer[j++] = (uint8_t)'S';
	txBuffer[j++] = timeinfo2.tm_year - 2000;
	txBuffer[j++] = timeinfo2.tm_mon;
	txBuffer[j++] = timeinfo2.tm_mday;
	txBuffer[j++] = timeinfo2.tm_wday;
	txBuffer[j++] = timeinfo2.tm_hour;
	txBuffer[j++] = timeinfo2.tm_min;
	txBuffer[j++] = timeinfo2.tm_sec;
	txBuffer[j++] = msec >> 8;
	txBuffer[j++] = msec & 0xff;
	return j - PACKET_PRE_BYTES;
}

uint16_t Command_PrepareTemperature(uint8_t *txBuffer, uint16_t txBufferLength)
{
	uint16_t j = PACKET_PRE_BYTES;
	txBuffer[j++] = (uint8_t)'t';
	return j - PACKET_PRE_BYTES;
}

uint16_t Command_PrepareRelaysStatus(uint8_t *txBuffer, uint16_t txBufferLength)
{
	uint16_t j = PACKET_PRE_BYTES;
	txBuffer[j++] = (uint8_t)'r';
	return j - PACKET_PRE_BYTES;
}

uint16_t Command_PrepareRelaySetState(uint8_t *txBuffer, uint16_t txBufferLength,
		uint8_t relay, bool state)
{
	uint16_t j = PACKET_PRE_BYTES;
	txBuffer[j++] = (uint8_t)'r';
	txBuffer[j++] = relay;
	txBuffer[j++] = state ? 1 : 0;
	return j - PACKET_PRE_BYTES;
}

void Command_AnswerGetRelaysStatus(uint32_t address, bool answerOk, uint32_t uptime, uint16_t vin, bool *relays, uint8_t count) __attribute__((weak));
void Command_AnswerSetRelaysStatus(uint32_t address, bool answerOk) __attribute__((weak));
void Command_AnswerGetTemperatures(uint32_t address, bool answerOk, uint32_t uptime, uint16_t vin, uint16_t *temperatures, uint8_t count) __attribute__((weak));

void Command_DecodeAnswer(struct Communication* communication, uint16_t receivedBytes, uint8_t* rxBuffer)
{
	uint8_t* rxBuffer_ = (uint8_t*)(rxBuffer + PACKET_PRE_BYTES);
	uint32_t address = (rxBuffer[11] << 24) | (rxBuffer[12] << 16) | (rxBuffer[13] << 8) | rxBuffer[14];

		/// 'r' - Get Relays Status
	if (receivedBytes >= 8 && rxBuffer_[0] == (char)'r')
	{
		uint32_t uptime = (rxBuffer_[1] << 24) | (rxBuffer_[2] << 16) | (rxBuffer_[3] << 8) | rxBuffer_[4];
		uint16_t vin = (rxBuffer_[5] << 8) | rxBuffer_[6];
		uint8_t count = rxBuffer_[7];
		bool relays[8];
		memset(relays, (uint8_t)false, sizeof(relays));
		bool ok = receivedBytes == count + 8;
		uint8_t j = 0;
		if (ok)
			for (uint8_t i = 0; i < count; i++)
				relays[j++] = rxBuffer_[i + 8] != 0;
		Command_AnswerGetRelaysStatus(address, ok, uptime, vin, relays, count);
	}

		/// 'r' - Set Relay State
	else if (receivedBytes == 2 && rxBuffer_[0] == (char)'r')
	{
		bool ok = rxBuffer_[1] != 0;
		Command_AnswerSetRelaysStatus(address, ok);
	}

		/// 't' - Temperature
	else if (receivedBytes >= 8 && rxBuffer_[0] == (char)'t')
	{
		uint32_t uptime = (rxBuffer_[1] << 24) | (rxBuffer_[2] << 16) | (rxBuffer_[3] << 8) | rxBuffer_[4];
		uint16_t vin = (rxBuffer_[5] << 8) | rxBuffer_[6];
		uint8_t count = rxBuffer_[7];
		bool ok = receivedBytes == count * sizeof(uint16_t) + 8;
		uint8_t j = 0;
		uint16_t temperatures[8];
		memset(temperatures, 0xff, sizeof(temperatures));
		if (ok)
			for (uint8_t i = 0; i < count * 2; i += 2)
				temperatures[j++] = (rxBuffer_[i + 8] << 8) | rxBuffer_[i + 8 + 1];
		Command_AnswerGetTemperatures(address, ok, uptime, vin, temperatures, count);
	}
}
