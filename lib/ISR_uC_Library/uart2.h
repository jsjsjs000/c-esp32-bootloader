
#include "../../include/main.h"

#ifdef ISR_DIN
	#define UART2_PIN_TXD 17
	#define UART2_PIN_RXD 35
	#define UART2_PIN_RTS 16
#endif
#ifdef ISR_BOX
	#define UART2_PIN_TXD 17
	#define UART2_PIN_RXD 16
	#define UART2_PIN_RTS 22
#endif
#define UART2_PIN_CTS UART_PIN_NO_CHANGE
#define UART2_RS485

#define UART2_PORT_NUM      1
#define UART2_BAUD_RATE     115200

extern struct Communication communication2;

extern void InitializeUart2(void);
extern void Uart2Send(uint8_t* buffer, uint16_t bufferSize);
extern bool Uart2SendWithEncode(uint8_t* buffer, uint16_t bufferSize, uint32_t destinationAddress,
	uint8_t sendCommand);
