#define SEND_COMMAND_TIMEOUT            20

#define SEND_COMMAND_NONE               0
#define SEND_COMMAND_GET_TEMPERATURE    1
#define SEND_COMMAND_GET_RELAY_STATE    2
#define SEND_COMMAND_SET_RELAY_STATE    3

extern uint16_t Command_PrepareSynchronization(uint8_t *txBuffer, uint16_t txBufferLength);
extern uint16_t Command_PrepareTemperature(uint8_t *txBuffer, uint16_t txBufferLength);
extern uint16_t Command_PrepareRelaysStatus(uint8_t *txBuffer, uint16_t txBufferLength);
extern uint16_t Command_PrepareRelaySetState(uint8_t *txBuffer, uint16_t txBufferLength,
		uint8_t relay, bool state);

extern void Command_AnswerGetRelaysStatus(uint32_t address, bool answerOk, uint32_t uptime, uint16_t vin, bool *relays, uint8_t count);
extern void Command_AnswerSetRelaysStatus(uint32_t address, bool answerOk);
extern void Command_AnswerGetTemperatures(uint32_t address, bool answerOk, uint32_t uptime, uint16_t vin, uint16_t *temperatures, uint8_t count);
extern void Command_DecodeAnswer(struct Communication* communication, uint16_t receivedBytes, uint8_t* rxBuffer);
