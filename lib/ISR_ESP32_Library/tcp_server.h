#define TCP_SERVER_PORT                        28844
#define TCP_SERVER_KEEPALIVE_IDLE              5				// seconds
#define TCP_SERVER_KEEPALIVE_INTERVAL          5				// seconds
#define TCP_SERVER_KEEPALIVE_COUNT             3
#define TCP_SERVER_MAX_TCP_CLIENT_CONNECTIONS  5
#define TCP_SERVER_BUFFER_RX_LENGTH            1024
#define TCP_SERVER_BUFFER_TX_LENGTH            1024

extern void TcpServer_Send(int socket, uint8_t* txBuffer, uint16_t txBufferLength);
extern void TcpServer_CreateServer(void);
