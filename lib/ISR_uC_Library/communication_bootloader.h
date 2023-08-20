#ifndef INC_COMMUNICATION_BOOTLOADER_H_
#define INC_COMMUNICATION_BOOTLOADER_H_

#include "../../include/main.h"
#include "communication.h"

#define PROGRAMMING_SEND_MAX_NOT_RESOLVED_PACKETS 20
#define PROGRAMMING_PACKET_SIZE                   256
#ifdef STM32F0
	#define PROGRAMMING_MAX_PACKETS                 144		/// 36KB / 256 = 144
#endif
#ifdef ESP32
	#define PROGRAMMING_MAX_PACKETS                 1792	/// 448KB / 256 = 1792
#endif

#define COMMUNICATION_COMMAND_NONE                          0
#define COMMUNICATION_COMMAND_CLEAR_FLASH                   1
#define COMMUNICATION_COMMAND_RESET                         2
#define COMMUNICATION_COMMAND_JUMP_TO_PROGRAM               3
#define COMMUNICATION_COMMAND_GET_ADDRESS_WITH_RANDOM_DELAY 4
#define COMMUNICATION_COMMAND_END_DEVICE_PROGRAMMING        5
#define COMMUNICATION_COMMAND_SET_DEVICE_ADDRESS            6

extern uint16_t ExecuteCommunicationCommand_Bootloader(struct Communication *communication,
		uint8_t *txBuffer, uint16_t txBufferLength);
extern void ResetResolvedPackets(void);
extern uint16_t ReceiveForRequest_Bootloader(struct Communication *communication,
		uint16_t receivedBytes, uint8_t *rxBuffer, uint8_t *txBuffer, uint16_t txBufferLength);

#endif /* INC_COMMUNICATION_BOOTLOADER_H_ */
