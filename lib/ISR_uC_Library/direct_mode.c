#include <stdio.h>
#include "direct_mode.h"

void InitializeDirectModeBuffer(struct DirectModeBuffer* directModeBuffer)
{
	directModeBuffer->count = 0;
	directModeBuffer->bytesToTransmit = 0;
	directModeBuffer->isTransmit = 0;
}
