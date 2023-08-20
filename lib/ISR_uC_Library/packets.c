#include "../../include/main.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#ifdef STM32F0
	#include "stm32f0xx_hal.h"
#endif
#ifdef STM32F4
	#include "stm32f4xx_hal.h"
#endif
#include "packets.h"
#include "crc32.h"
#include "communication.h"

uint16_t EncodePacket(uint8_t* txBuffer, uint16_t txBufferLength, uint32_t packetId,
		uint32_t encryptionKey, uint32_t address, uint8_t isAnswer, uint16_t bytesToSend)
{
	if (bytesToSend == 0 || bytesToSend + PACKET_PRE_BYTES + PACKET_POST_BYTES > txBufferLength)
		return 0;

	txBuffer[0] = SOP;
	txBuffer[1] = bytesToSend & 0xff;
	txBuffer[2] = (bytesToSend >> 8) & 0x3f;
	if (isAnswer != 0)
		txBuffer[2] |= 0x80;
	txBuffer[3] = packetId >> 24;
	txBuffer[4] = (packetId >> 16) & 0xff;
	txBuffer[5] = (packetId >> 8) & 0xff;
	txBuffer[6] = packetId & 0xff;
	txBuffer[7] = encryptionKey >> 24;
	txBuffer[8] = (encryptionKey >> 16) & 0xff;
	txBuffer[9] = (encryptionKey >> 8) & 0xff;
	txBuffer[10] = encryptionKey & 0xff;
	txBuffer[11] = address >> 24;
	txBuffer[12] = (address >> 16) & 0xff;
	txBuffer[13] = (address >> 8) & 0xff;
	txBuffer[14] = address & 0xff;
	uint32_t crc32 = CalculateCrc32(0, (uint8_t*)txBuffer, 1, PACKET_PRE_BYTES - 1 + bytesToSend);
	txBuffer[PACKET_PRE_BYTES + bytesToSend + PACKET_POST_BYTES - 5] = (crc32 >> 24) & 0xff;
	txBuffer[PACKET_PRE_BYTES + bytesToSend + PACKET_POST_BYTES - 4] = (crc32 >> 16) & 0xff;
	txBuffer[PACKET_PRE_BYTES + bytesToSend + PACKET_POST_BYTES - 3] = (crc32 >> 8) & 0xff;
	txBuffer[PACKET_PRE_BYTES + bytesToSend + PACKET_POST_BYTES - 2] = crc32 & 0xff;
	txBuffer[PACKET_PRE_BYTES + bytesToSend + PACKET_POST_BYTES - 1] = EOP;
	return PACKET_PRE_BYTES + bytesToSend + PACKET_POST_BYTES;
}

bool CheckCorrectFrameInBuffer(uint8_t* rxBuffer, uint16_t *rxBufferCount, bool* isAnswer)
{
	while (*rxBufferCount > 0 && rxBuffer[0] != SOP) /// first byte must be SOP
	{
		*rxBufferCount = 0;
		return false;
	}

	if (*rxBufferCount <= EMPTY_FRAME_LENGTH) /// frame must have minimum bytes
		return false;

	uint16_t dataLength = rxBuffer[1] | ((rxBuffer[2] & 0x7f) << 8);
	if (dataLength == 0 || dataLength > MAX_FRAME_LENGTH) /// read data length and check it
	{
		*rxBufferCount = 0;
		return false;
	}

	uint16_t frameLength = PACKET_PRE_BYTES + dataLength + PACKET_POST_BYTES;
	if (*rxBufferCount < frameLength) /// frame not received complete yet
		return false;

	if (rxBuffer[*rxBufferCount - 1] != EOP) /// EOP not exists
	{
		rxBuffer[0] = rxBuffer[*rxBufferCount - 1];
		*rxBufferCount = 1;
		return false;
	}

	*isAnswer = (rxBuffer[2] & 0x80) != 0;

	uint32_t readCrc = (rxBuffer[*rxBufferCount - 5] << 24) | (rxBuffer[*rxBufferCount - 4] << 16) |
			(rxBuffer[*rxBufferCount - 3] << 8) | rxBuffer[*rxBufferCount - 2];
	uint32_t calculatedCrc = CalculateCrc32(0, rxBuffer, 1, PACKET_PRE_BYTES - 1 + dataLength);
	if (readCrc == calculatedCrc)
		return true;

	if (*rxBufferCount >= frameLength) /// incorrect frame - reset buffer
		*rxBufferCount = 0;

	return false;
}

// return 0 - answer not found
// rxBufferCount = 0 - buffer cleared and wait for SOP byte
uint16_t CheckFrameInBuffer(struct Communication *communication, uint8_t* rxBuffer,
		uint16_t *rxBufferCount, uint32_t packetId, uint32_t address, bool* isAnswer)
{
	if (!CheckCorrectFrameInBuffer(rxBuffer, rxBufferCount, isAnswer))
		return 0;

	uint16_t dataLength = rxBuffer[1] | ((rxBuffer[2] & 0x7f) << 8);
	uint32_t packetIdRead = (rxBuffer[3] << 24) | (rxBuffer[4] << 16) | (rxBuffer[5] << 8) | rxBuffer[6];
	uint32_t encryptionKeyRead = (rxBuffer[7] << 24) | (rxBuffer[8] << 16) | (rxBuffer[9] << 8) | rxBuffer[10];
	uint32_t addressRead = (rxBuffer[11] << 24) | (rxBuffer[12] << 16) | (rxBuffer[13] << 8) | rxBuffer[14];
	if (!(addressRead == address || addressRead == BROADCAST ||
			packetIdRead == packetId || packetIdRead == BROADCAST ||
			(isAnswer && address == BROADCAST)))
	{
		*rxBufferCount = 0; /// crc or class or address not correct
		return 0;
	}

	communication->packetId = packetIdRead;
	communication->encryptionKey = encryptionKeyRead;
	communication->address = addressRead;
	*rxBufferCount = 0;
	return dataLength;
}
