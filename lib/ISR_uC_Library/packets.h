#ifndef __PACKETS_H
#define __PACKETS_H

#include "../../include/main.h"
#ifdef STM32F0
	#include "stm32f0xx_hal.h"
#endif
#ifdef STM32F4
	#include "stm32f4xx_hal.h"
#endif
#include "communication.h"

#define SOP					        0xA0
#define EOP					        0xA1
#define BROADCAST		        0xffffffff
#define PACKET_PRE_BYTES    15
#define PACKET_POST_BYTES   5
#define EMPTY_FRAME_LENGTH	(PACKET_PRE_BYTES + PACKET_POST_BYTES)
#define MAX_FRAME_LENGTH		(1024 - EMPTY_FRAME_LENGTH - 1)

extern uint16_t EncodePacket(uint8_t* txBuffer, uint16_t txBufferLength, uint32_t packetId,
		uint32_t encryptionKey, uint32_t address, uint8_t isAnswer, uint16_t bytesToSend);
extern bool CheckCorrectFrameInBuffer(uint8_t* rxBuffer, uint16_t *rxBufferCount, bool* isAnswer);
extern uint16_t CheckFrameInBuffer(struct Communication *communication, uint8_t* rxBuffer,
		uint16_t *rxBufferCount, uint32_t packetId, uint32_t address, bool* isAnswer);

#endif /* __PACKETS_H */
