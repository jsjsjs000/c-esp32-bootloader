#include "../../include/main.h"

#define UART1_PIN_TXD 1
#define UART1_PIN_RXD 3
#define UART1_PIN_RTS 2
#define UART1_PIN_CTS UART_PIN_NO_CHANGE
#define UART1_RS485

#define UART1_PORT_NUM      0
#define UART1_BAUD_RATE     115200

extern struct Communication communication1;

extern void InitializeUart1(void);
extern void Uart1Send(uint8_t* buffer, uint16_t bufferSize);
extern bool Uart1SendWithEncode(uint8_t* buffer, uint16_t bufferSize, uint32_t destinationAddress,
    uint8_t sendCommand);
