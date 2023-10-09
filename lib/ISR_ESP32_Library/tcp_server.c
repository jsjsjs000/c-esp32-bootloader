/// https://github.com/espressif/esp-idf/blob/1067b28707e527f177752741e3aa08b5dc64a4d7/examples/protocols/sockets/tcp_server/main/tcp_server.c

#include "../../include/main.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "tcp_server.h"

static const char *TAG = "tcp_server";
static const uint8_t LogLevel = 0; /// 0 - 2

static bool tcpServerCreated = false;
EventGroupHandle_t tcpEventGroup;
TaskHandle_t tasks[TCP_SERVER_MAX_TCP_CLIENT_CONNECTIONS] = { 0 };
int tasksSocks[TCP_SERVER_MAX_TCP_CLIENT_CONNECTIONS] = { 0 };

uint16_t TcpServer_AnswerForRequest(int32_t socket, uint8_t* rxBuffer, uint16_t rxBufferLength,
		uint8_t* txBuffer, uint16_t txBufferLength) __attribute__((weak));

void do_retransmit(void *pvParameters)
{
	int receivedBytes;
	uint8_t rxBuffer[TCP_SERVER_BUFFER_RX_LENGTH];
	uint8_t txBuffer[TCP_SERVER_BUFFER_TX_LENGTH];
	int taskId = *(int*)pvParameters;
	int socket = tasksSocks[taskId];

	do
	{
		receivedBytes = recv(socket, rxBuffer, sizeof(rxBuffer) - 1, 0);
		if (receivedBytes < 0)
		{
			if (LogLevel >= 1)
				LOGE(TAG, "Error occurred during receiving: errno %d", errno);
		}
		else if (receivedBytes == 0)
		{
			if (LogLevel >= 1)
				LOGI(TAG, "Connection closed, TCP client slot %d", taskId);
		}
		else
		{
			if (LogLevel >= 2)
				LOGI(TAG, "Received %d bytes: %02x %02x %02x ...", receivedBytes, rxBuffer[0], rxBuffer[1], rxBuffer[2]);

				// send() can return less bytes than supplied length.
				// Walk-around for robust implementation.
			uint16_t sendBytes = TcpServer_AnswerForRequest(socket, rxBuffer, receivedBytes, txBuffer, sizeof(txBuffer));
			uint16_t toWrite = sendBytes;

			if (LogLevel >= 2)
			{
				if (toWrite > 0)
					LOGI(TAG, "Answer %d bytes: %02x %02x %02x ...", toWrite, txBuffer[0], txBuffer[1], txBuffer[2]);
				LOGI(TAG, "Socket %d", socket);
			}

			while (toWrite > 0)
			{
				int written = send(socket, txBuffer + (sendBytes - toWrite), toWrite, 0);
				if (LogLevel >= 1 && written < 0)
					LOGE(TAG, "Error occurred during sending: errno %d", errno);
				toWrite -= written;
			}

			if (LogLevel >= 2)
			{
				int stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
				int freeHeapSize = xPortGetFreeHeapSize();
				LOGI(TAG, "stackHighWaterMark %d, freeHeapSize %d, uptime %d s", stackHighWaterMark, freeHeapSize, GET_MS() / 1000);
			}
		}
	}
	while (receivedBytes > 0);

	shutdown(socket, 0);
	close(socket);

	xEventGroupSetBits(tcpEventGroup, 1 << taskId);
	vTaskDelete(NULL);
}

void TcpServer_Send(int socket, uint8_t* txBuffer, uint16_t txBufferLength)
{
	if (LogLevel >= 2)
		LOGI(TAG, "Send %d bytes: %02x %02x %02x ...", txBufferLength, txBuffer[0], txBuffer[1], txBuffer[2]);

	uint16_t sendBytes = txBufferLength;
	while (txBufferLength > 0)
	{
		int written = send(socket, txBuffer + (sendBytes - txBufferLength), txBufferLength, 0);
		if (LogLevel >= 1 && written < 0)
			LOGE(TAG, "Error occurred during sending: errno %d", errno);
		txBufferLength -= written;
	}
}

void tcp_server_task(void *pvParameters)
{
	char addrStr[128];
	int addrFamily = (int)pvParameters;
	int ipProtocol = 0;
	int keepAlive = 1;
	int keepIdle = TCP_SERVER_KEEPALIVE_IDLE;
	int keepInterval = TCP_SERVER_KEEPALIVE_INTERVAL;
	int keepCount = TCP_SERVER_KEEPALIVE_COUNT;
	struct sockaddr_storage destAddr;

	if (addrFamily == AF_INET)
	{
		struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&destAddr;
		dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
		dest_addr_ip4->sin_family = AF_INET;
		dest_addr_ip4->sin_port = htons(TCP_SERVER_PORT);
		ipProtocol = IPPROTO_IP;
	}

	int listenSocket = socket(addrFamily, SOCK_STREAM, ipProtocol);
	if (listenSocket < 0)
	{
		if (LogLevel >= 1)
			LOGE(TAG, "Unable to create socket: errno %d", errno);
		vTaskDelete(NULL);
		return;
	}
	int opt = 1;
	setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if (LogLevel >= 1)
		LOGI(TAG, "Socket created");

	int err = bind(listenSocket, (struct sockaddr *)&destAddr, sizeof(destAddr));
	if (err != 0)
	{
		if (LogLevel >= 1)
		{
			LOGE(TAG, "Socket unable to bind: errno %d", errno);
			LOGE(TAG, "IPPROTO: %d", addrFamily);
		}
		goto CLEAN_UP;
	}
	if (LogLevel >= 1)
		LOGI(TAG, "Socket bound, port %d", TCP_SERVER_PORT);

	err = listen(listenSocket, 1);
	if (err != 0)
	{
		if (LogLevel >= 1)
			LOGE(TAG, "Error occurred during listen: errno %d", errno);
		goto CLEAN_UP;
	}

	DELAY_MS(100);

	while (true)
	{
		if (LogLevel >= 1)
			LOGI(TAG, "Socket listening");

		struct sockaddr_storage sourceAddr;
		socklen_t addrLen = sizeof(sourceAddr);
		int socket = accept(listenSocket, (struct sockaddr *)&sourceAddr, &addrLen);
		if (socket < 0)
		{
			if (LogLevel >= 1)
				LOGE(TAG, "Unable to accept connection: errno %d", errno);
			break;
		}

			// Set tcp keepalive option
		setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
		setsockopt(socket, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
		setsockopt(socket, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
		setsockopt(socket, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
			// Convert ip address to string
		if (sourceAddr.ss_family == PF_INET)
			inet_ntoa_r(((struct sockaddr_in *)&sourceAddr)->sin_addr, addrStr, sizeof(addrStr) - 1);

		if (LogLevel >= 1)
			LOGI(TAG, "Socket accepted ip address: %s", addrStr);

		EventBits_t bits = xEventGroupWaitBits(tcpEventGroup, (1 << TCP_SERVER_MAX_TCP_CLIENT_CONNECTIONS) - 1,
				false, false, pdMS_TO_TICKS(100));

		if (bits == 0)
		{
			if (LogLevel >= 1)
				LOGI(TAG, "No free slot for TCP client - disconnected");

			shutdown(socket, 0);
			close(socket);
		}
		else
		{
			for (int i = 0; i < TCP_SERVER_MAX_TCP_CLIENT_CONNECTIONS; i++)
				if ((bits & (1 << i)) != 0)
				{
					if (LogLevel >= 1)
						LOGI(TAG, "Free TCP client slot %d", i);

					xEventGroupClearBits(tcpEventGroup, 1 << i);
					tasksSocks[i] = socket;
					TaskHandle_t handle = NULL;
					char taskName[20];
					sprintf(taskName, "tcp_task%d", i);
					if (xTaskCreate(do_retransmit, taskName, 4096, &i, 5, &handle))
					{
						tasks[i] = handle;
						break;
					}
				}
		}

		DELAY_MS(1);
	}

CLEAN_UP:
	close(listenSocket);
	vTaskDelete(NULL);
}

void TcpServer_CreateServer(void)
{
	if (tcpServerCreated)
		return;

	tcpEventGroup = xEventGroupCreate();
	xEventGroupSetBits(tcpEventGroup, (1 << TCP_SERVER_MAX_TCP_CLIENT_CONNECTIONS) - 1);
	xTaskCreate(tcp_server_task, "tcp_server", 8192, (void*)AF_INET, 5, NULL);
	tcpServerCreated = true;
}
