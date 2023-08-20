// extern void InitializeCrc32Table();
extern uint32_t CalculateCrc32(uint32_t crc, uint8_t* data, int16_t from, int16_t length);
extern uint32_t CalculateCrc32_1Byte(uint32_t crc, uint8_t data);
extern uint32_t CalculateCrc32_1UInt(uint32_t crc, uint32_t data);
