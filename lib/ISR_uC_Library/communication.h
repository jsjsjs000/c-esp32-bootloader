#ifndef INC_COMMUNICATION_REL_H_
#define INC_COMMUNICATION_REL_H_

#include "configuration.h"

#define COMMUNICATION_BUFFER_RX_LENGTH				1024
#define COMMUNICATION_BUFFER_TX_LENGTH				1024

#define COMMUNICATION_COMMAND_NONE                          0
#define COMMUNICATION_COMMAND_CLEAR_FLASH                   1
#define COMMUNICATION_COMMAND_RESET                         2
#define COMMUNICATION_COMMAND_JUMP_TO_PROGRAM               3
#define COMMUNICATION_COMMAND_GET_ADDRESS_WITH_RANDOM_DELAY 4
#define COMMUNICATION_COMMAND_END_DEVICE_PROGRAMMING        5
#define COMMUNICATION_COMMAND_SET_DEVICE_ADDRESS            6
#define COMMUNICATION_COMMAND_DIRECT_MODE_OFF               7
#define COMMUNICATION_COMMAND_DIRECT_MODE_ON                8

#define PACKET_SOURCE_NONE         0
#define PACKET_SOURCE_ETHERNET     1
#define PACKET_SOURCE_UART1        2
#define PACKET_SOURCE_UART2        3
#define PACKET_SOURCE_UART3        4
#define PACKET_SOURCE_UART4        5

#define DIRECT_MODE_DELAY             30  // ms
#define DIRECT_MODE_MAX_MILLISECONDS  (10 * 60 * 1000)

#define DIRECT_MODE_SOURCE_OFF       0
#define DIRECT_MODE_SOURCE_ETHERNET  PACKET_SOURCE_ETHERNET
#define DIRECT_MODE_SOURCE_UART1     PACKET_SOURCE_UART1
#define DIRECT_MODE_SOURCE_UART2     PACKET_SOURCE_UART2
#define DIRECT_MODE_SOURCE_UART3     PACKET_SOURCE_UART3
#define DIRECT_MODE_SOURCE_UART4     PACKET_SOURCE_UART4

#define DIRECT_MODE_TARGET_ALL       0xff
#define DIRECT_MODE_TARGET_UART1     1
#define DIRECT_MODE_TARGET_UART2     2
#define DIRECT_MODE_TARGET_UART3     3
#define DIRECT_MODE_TARGET_UART4     4

#define LINE_NONE         0
#define LINE_UART1        1
#define LINE_UART2        2
#define LINE_UART3        3
#define LINE_UART4        4
#define LINE_RADIO        64
#define LINE_LAN          65

struct Communication
{
	uint8_t lineNumber;

	uint8_t command;
	uint32_t commandParam1;
	uint32_t commandParam2;
	uint32_t commandFA_blockTime;
	uint32_t commandFA_replayDelay;
	StatusType commandStatus;
	uint16_t commandPagesToClear;
	uint32_t commandProgramCrc32;
	uint8_t commandSetHardwareType1;
	uint8_t commandSetHardwareType2;
	uint8_t commandSetHardwareSegmentsCount;
	uint8_t commandSetHardwareVersion;
	uint32_t commandSetAddress;
	uint32_t lastSent;
	uint32_t lastReceived;
	uint8_t flashBuffer[FLASH_PAGE_SIZE];
	int32_t socket;
	uint8_t directModeSource;
	uint8_t directModeTarget;
	uint8_t lastPacketSource;
	uint32_t packetId;
	uint32_t encryptionKey;
	uint32_t address;

#ifdef ESP32
	uint16_t bufferRxIndex;
	uint8_t bufferRx[COMMUNICATION_BUFFER_RX_LENGTH];
	uint8_t bufferTx[COMMUNICATION_BUFFER_TX_LENGTH];
#endif

#ifdef ISR_CU
	uint8_t sendCommand;
	uint32_t sendCommandAddress;
	uint16_t sendCommandBytesToSend;
	uint8_t sendCommandSegmentsCount;
	uint32_t lastSendCommandTime;
	uint32_t lastSendCommandPacketId;
	uint32_t lastSendCommandEncryptionKey;
	uint32_t lastSendCommandAddress;
#endif
};

extern void InitializeCommunication(struct Communication *communication, uint8_t lineNumber);
extern uint16_t ExecuteCommunicationCommand(struct Communication *communication, uint8_t *txBuffer, uint16_t txBufferLength);
extern uint16_t ReceiveForRequest(struct Communication *communication, uint16_t receivedBytes, uint8_t *rxBuffer,
		uint8_t *txBuffer, uint16_t txBufferLength, uint8_t packetSource);

#endif /* INC_COMMUNICATION_REL_H_ */
