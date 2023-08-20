#include <stdio.h>

#define UART_DIRECT_MODE_BUFFER_LENGHT  64

struct DirectModeBuffer
{
  uint8_t byte;
	uint8_t count;						// from hubs line to sensors line 1
	uint8_t buffer[UART_DIRECT_MODE_BUFFER_LENGHT];	// rotate buffer
	uint8_t bytesToTransmit;
	uint8_t isTransmit;
};

extern void InitializeDirectModeBuffer(struct DirectModeBuffer* directModeBuffer);
