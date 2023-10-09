#include "../../include/main.h"
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "device_item.h"
#include "flash.h"
#include "history_state.h"
#include "rtc.h"

// #define HOST "jarsulk.pl"
#define HOST_IP_ADDR "192.168.0.10"
// #define HOST_IP_ADDR "10.1.4.40"
// #define HOST_IP_ADDR "51.38.128.112" // jarsulk.pl
#define PORT 28501		// object 1 - Adam Kukuc
#define TCP_RECEIVE_TIMEOUT_MS  2000

static const char *TAG = "tcp_client";
static const uint8_t LogLevel = 0; /// 0 - 2 $$$

TickType_t last_send_status = 0;

static size_t tcp_client_receive(int sock, uint8_t rx_buffer[], size_t rx_buffer_size);

static void tcp_client_task(void *pvParameters)
{
	ip_addr_t domain_addr;
	struct addrinfo domain_hint;
	struct addrinfo *domain_res = NULL;

	uint8_t rx_buffer[1024];
	uint8_t tx_buffer[1024];
	char host_ip[] = HOST_IP_ADDR;
	int addr_family = 0;
	int ip_protocol = 0;
	TickType_t last_try_send_status = 0;

	while (true)
	{
			// hostname to ip4
		memset(&domain_hint, 0, sizeof(domain_hint));
		memset(&domain_addr, 0, sizeof(domain_addr));
		if (getaddrinfo(HOST_IP_ADDR, NULL, &domain_hint, &domain_res) != 0)
		{
			if (LogLevel >= 2)
				LOGI(TAG, "IP4: error");
			DELAY_MS(1000);
			continue;
		}
		else
		{
			struct in_addr addr4 = ((struct sockaddr_in*)(domain_res->ai_addr))->sin_addr;
			inet_addr_to_ip4addr(ip_2_ip4(&domain_addr), &addr4);
			ip4_addr_t ip4 = domain_addr.u_addr.ip4;
			if (LogLevel >= 2)
				LOGI(TAG, "%s = %d.%d.%d.%d", HOST_IP_ADDR, ip4_addr1_val(ip4), ip4_addr2_val(ip4),
						ip4_addr3_val(ip4), ip4_addr4_val(ip4));
		}
		freeaddrinfo(domain_res);

		struct sockaddr_in dest_addr;
		// dest_addr.sin_addr.s_addr = inet_addr(host_ip);
		dest_addr.sin_addr.s_addr = domain_addr.u_addr.ip4.addr;
		dest_addr.sin_family = AF_INET;
		dest_addr.sin_port = htons(PORT);
		addr_family = AF_INET;
		ip_protocol = IPPROTO_IP;
		int sock = socket(addr_family, SOCK_STREAM, ip_protocol);
		if (sock < 0)
		{
			if (LogLevel > 0)
				LOGE(TAG, "Unable to create socket: errno %d", errno);
			DELAY_MS(1000);
			continue;
		}
		if (LogLevel >= 2)
			LOGI(TAG, "Socket created, connecting to %s:%d", host_ip, PORT);

			/// Set receive timeout to 10ms
		struct timeval tv = {
			.tv_sec = 0,
			.tv_usec = 10000
		};
		int err = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (err != 0)
		{
			if (LogLevel > 0)
				LOGE(TAG, "setsockopt failed: errno %d", errno);
		}

		err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
		if (err != 0)
		{
			if (LogLevel > 0)
				LOGE(TAG, "Socket unable to connect: errno %d", errno);
			shutdown(sock, 0);
			close(sock);
			DELAY_MS(1000);
			continue;
		}
		
		if (LogLevel >= 2)
			LOGI(TAG, "Successfully connected");

		while (true)
		{
			if (GET_MS() - last_try_send_status >= 2000)
			{
					/// get status once per 2s
				uint16_t fromItem = 0;
				uint8_t details = 1;
				uint16_t send_bytes = DeviceItem_GetStatus(&devicesItems, devicesItemsStatus, heatingDevicesComponents,
						&tx_buffer[1], sizeof(tx_buffer) - 1, fromItem, details, 0); // - PACKET_PRE_BYTES - PACKET_POST_BYTES
				tx_buffer[0] = 'g';
				send_bytes++;
// if (LogLevel >= 2) LOGI(TAG, "status send_bytes %d", send_bytes);

					/// send status to server
				err = send(sock, tx_buffer, send_bytes, 0);
				if (err < 0)
				{
					if (LogLevel > 0)
						LOGE(TAG, "Error occurred during sending: errno %d", errno);
					break;
				}
				size_t received = tcp_client_receive(sock, rx_buffer, sizeof(rx_buffer));
				if (received >= 2 && rx_buffer[0] == 'g' && rx_buffer[1] == 0)
				{
					last_send_status = GET_MS();
// if (LogLevel >= 2) LOGI(TAG, "Send status to server: ok");
				}

					/// try send history state to server from flash
				if (FindNotSendHistoryState(&flashHistoryState))
				{
// if (LogLevel >= 2)
// LOGI(TAG, "FindNotSendHistoryState - cur: %x, notsend: %x", flashHistoryState.currentFlashAddress, flashHistoryState.notSendCurrentItemAddress); // $$$
					if (flashHistoryState.currentFlashAddress != flashHistoryState.notSendCurrentItemAddress)
					{
char s[32];
RTC_GetCurrentDateTimeString(s, sizeof(s));
LOGI(TAG, "Found item in flash history - send to server - %s - flashHistoryState.currentFlashAddress %x", s, flashHistoryState.notSendCurrentItemAddress); // $$$

						if (ReadFromFlash(flashHistoryState.notSendCurrentItemAddress, GetHistoryStateItemSize(&flashHistoryState),
								tx_buffer) == STATUS_OK)
						{
// $$ nie z sendHistoryStateBuffer tylko odczytać z flasha
// if (LogLevel >= 2) LOGI(TAG, "send history %x %d", sendHistoryStateBuffer[0], sendHistoryStateBufferCount);
							size_t offset = 1 + FLASH_HISTORY_STATE_HEADER_EXTRA_BYTES - 1;		/// flash_signature + header_extra - 'g'
							tx_buffer[offset] = 'g';
							size_t send_bytes_count = 1 + FLASH_HISTORY_STATE_ITEM_SIZE;			/// + 'g'
if (LogLevel >= 2) LOGI(TAG, "  Send %d", send_bytes_count);
							err = send(sock, &tx_buffer[offset], send_bytes_count, 0);
							size_t received = tcp_client_receive(sock, rx_buffer, sizeof(rx_buffer));
if (LogLevel >= 2) LOGI(TAG, "  Received %d B: [%c %x]", received, rx_buffer[0], rx_buffer[1]);
							if (received >= 2 && rx_buffer[0] == 'g' && rx_buffer[1] == 0)
							{
if (LogLevel >= 2) LOGI(TAG, "SetSendHistoryState ok");
								SetSendHistoryState(&flashHistoryState);
							}
						}
					}
				}

				last_try_send_status = GET_MS();
			}

				/// receive commands from server
			ssize_t len = recv(sock, rx_buffer, sizeof(rx_buffer), MSG_DONTWAIT);
				/// Error occurred during receiving
			if (len < 0 && errno != EAGAIN)
			{
				if (LogLevel > 0)
					LOGE(TAG, "recv failed: errno %d, len %d", errno, len);
				break;
			}
				/// Data received
			else if (len > 0)
			{
				// if (LogLevel >= 2) LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
				// if (LogLevel >= 2) LOGI(TAG, "%x %x %x %x", rx_buffer[0], rx_buffer[1], rx_buffer[2], rx_buffer[3]);
				uint16_t send_bytes = TcpServer_AnswerForRequest(sock, rx_buffer, len, tx_buffer, sizeof(tx_buffer));
				send(sock, tx_buffer, send_bytes, 0);
				// if (LogLevel >= 2) LOGI(TAG, "Answer %d bytes, send %d bytes", send_bytes, len);
			}

			DELAY_MS(1);
		}

		if (sock != -1)
		{
			if (LogLevel > 0)
				LOGE(TAG, "Shutting down socket and restarting...");
			shutdown(sock, 0);
			close(sock);
			DELAY_MS(1000);
		}

	}

	vTaskDelete(NULL);
}

void TcpClient_CreateClient(void)
{
	xTaskCreate(tcp_client_task, "tcp_client", 6144, NULL, 5, NULL);
}

static size_t tcp_client_receive(int sock, uint8_t rx_buffer[], size_t rx_buffer_size)
{
	ssize_t len = 0;
	TickType_t dt = GET_MS();
	while (len <= 0 && GET_MS() - dt <= TCP_RECEIVE_TIMEOUT_MS)
	{
		ssize_t len2 = recv(sock, rx_buffer, rx_buffer_size, MSG_DONTWAIT);
		if (len2 > 0)
			len += len2;
		else
			DELAY_MS(5);
	}
// if (LogLevel >= 2) LOGI(TAG, "Received %d B, time: %d", len, GET_MS() - dt);

	return len;
}
