#include "../../include/main.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "common.h"
#include "configuration_bootloader.h"
#include "packets.h"
#include "communication.h"
#ifdef BOOTLOADER
	#include "communication_bootloader.h"
#endif
#if !defined(BOOTLOADER) && defined(ISR_CU)
	#include "commands.h"
	#include "central_unit_scan_devices.h"
	#include "device_item.h"
#endif
#include "uart1.h"
#include "uart2.h"
#include "tcp_server.h"

// static const char *TAG = "UART1";

static QueueHandle_t uart1Queue;
TickType_t lastUart1TaskTick = 0;
struct Communication communication1;
#if !defined(BOOTLOADER) && defined(ISR_CU)
	struct CentralUnitScanDevices scanDevices1;
#endif

void Uart1Send(uint8_t* buffer, uint16_t bufferSize)
{
	communication1.lastSent = GET_MS();
	uart_write_bytes(UART1_PORT_NUM, buffer, bufferSize);
	uart_wait_tx_done(UART1_PORT_NUM, 100);       /// 100ms timeout - 1KB
	uart_flush_input(UART1_PORT_NUM);
}

#ifdef ISR_CU
bool Uart1SendWithEncode(uint8_t* buffer, uint16_t bufferSize, uint32_t address,
		uint8_t sendCommand)
{
	communication1.lastSendCommandPacketId = Random32();
	communication1.lastSendCommandEncryptionKey = Random32();
	communication1.lastSendCommandAddress = address;
	uint16_t bytesToSend = EncodePacket(buffer, COMMUNICATION_BUFFER_TX_LENGTH, communication1.lastSendCommandPacketId,
			communication1.lastSendCommandEncryptionKey, address, false, bufferSize);
// LOGI("UART1", "encode %d bytes", bytesToSend);
	if (bytesToSend > 0)
	{
		communication1.sendCommand = sendCommand;
		communication1.bufferRxIndex = 0;
		communication1.lastSendCommandTime = GET_MS();
		Uart1Send(buffer, bytesToSend);
	}

// LOGI("UART1", "Send %d bytes: %02x %02x %02x ...", bytesToSend, buffer[0], buffer[1], buffer[2]);
// LOGI("UART1", "Send %d bytes to: %02x%02x%02x%02x '%c'", bufferSize, buffer[11], buffer[12], buffer[13], buffer[14], buffer[15]);
	return bytesToSend > 0;
}
#endif

void Uart1EventTask(void *arg)
{
	uart_event_t event;

	while (true)
	{
		if (xQueueReceive(uart1Queue, (void*)&event, pdMS_TO_TICKS(1)))
		{
			switch (event.type)
			{
				case UART_DATA:
				{
					int16_t canReadMax = MIN(event.size, COMMUNICATION_BUFFER_RX_LENGTH - communication1.bufferRxIndex);
					uint8_t *uartBufferRxPointer;
					uartBufferRxPointer = communication1.bufferRx + communication1.bufferRxIndex;
					int16_t receivedCount = uart_read_bytes(UART1_PORT_NUM, uartBufferRxPointer, canReadMax, portMAX_DELAY);
					if (!receivedCount)
						break;

					communication1.bufferRxIndex += receivedCount;

#if !defined(BOOTLOADER) && defined(ISR_CU)
					if (communication1.sendCommand != SEND_COMMAND_NONE)
						CentralUnitScanDevices_Receive(&communication1);
					else
					{
#endif
						bool isAnswer;
						uint16_t bytesReceivedCorrect = CheckFrameInBuffer(&communication1, communication1.bufferRx, &communication1.bufferRxIndex,
								BROADCAST, cfgBootloader.deviceAddress, &isAnswer);
						if (bytesReceivedCorrect > 0 && !isAnswer)
						{
							communication1.lastReceived = GET_MS();
							uint16_t bytesToSend = ReceiveForRequest(&communication1, bytesReceivedCorrect, communication1.bufferRx,
									communication1.bufferTx, sizeof(communication1.bufferTx), PACKET_SOURCE_UART1);
#ifdef BOOTLOADER
							if (bytesToSend == 0)
								bytesToSend = ReceiveForRequest_Bootloader(&communication1, bytesReceivedCorrect, communication1.bufferRx,
										communication1.bufferTx, sizeof(communication1.bufferTx));
#endif
							if (bytesToSend > 0)
								Uart1Send(communication1.bufferTx, bytesToSend);
						}
#if !defined(BOOTLOADER) && defined(ISR_CU)
					}
#endif

// LOGI("UART1", "Received %d bytes: %02x %02x %02x ...", receivedCount, communication1.bufferRx[0], communication1.bufferRx[1], communication1.bufferRx[2]);
// LOGI("UART1", "Received address: %02x%02x%02x%02x '%c'", communication1.bufferRx[11], communication1.bufferRx[12], communication1.bufferRx[13], communication1.bufferRx[14], communication1.bufferRx[15]);

#ifdef ISR_CU
					if (communicationTcp.directModeSource == DIRECT_MODE_SOURCE_ETHERNET)
					{
						communicationTcp.lastReceived = GET_MS();
						memcpy(communicationTcp.bufferTx, communication1.bufferRx, receivedCount);
						TcpServer_Send(communicationTcp.socket, communication1.bufferRx, receivedCount);
					}
#endif

					break;
				}
				case UART_BREAK:
					break;
				case UART_FIFO_OVF:
				case UART_BUFFER_FULL:
				case UART_PARITY_ERR:
				case UART_FRAME_ERR:
				case UART_PATTERN_DET:
				case UART_DATA_BREAK:
				case UART_EVENT_MAX:
					break;
			}
		}

		uint16_t bytesToSend = ExecuteCommunicationCommand(&communication1, communication1.bufferTx, sizeof(communication1.bufferTx));
#ifdef BOOTLOADER
		if (bytesToSend == 0)
			bytesToSend = ExecuteCommunicationCommand_Bootloader(&communication1, communication1.bufferTx, sizeof(communication1.bufferTx));
#endif
		if (bytesToSend > 0)
			Uart1Send(communication1.bufferTx, bytesToSend);

#if !defined(BOOTLOADER) && defined(ISR_CU)
		if (communicationTcp.directModeSource == DIRECT_MODE_SOURCE_OFF)
		{
			if (CentralUnitScanDevices_SendDevicesStates(&communication1))
				Uart1SendWithEncode(communication1.bufferTx, communication1.sendCommandBytesToSend,
						communication1.sendCommandAddress, communication1.sendCommand);
			else if (CentralUnitScanDevices_Loop(&communication1, &scanDevices1))
				Uart1SendWithEncode(communication1.bufferTx, communication1.sendCommandBytesToSend,
						communication1.sendCommandAddress, communication1.sendCommand);
		}
#endif

		lastUart1TaskTick = GET_MS();
		DELAY_MS(1);
	}
}

void InitializeUart1(void)
{
	InitializeCommunication(&communication1, LINE_UART1);
#if !defined(BOOTLOADER) && defined(ISR_CU)
	CentralUnitScanDevices_Initialize(&communication1, &scanDevices1);
#endif

	uart_config_t uart_config =
	{
		.baud_rate = UART1_BAUD_RATE,
		.data_bits = UART_DATA_8_BITS,
		.parity    = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.source_clk = UART_SCLK_APB,
	};

	ESP_ERROR_CHECK(uart_driver_install(UART1_PORT_NUM, 256, 256, 32, &uart1Queue, 0));
	ESP_ERROR_CHECK(uart_param_config(UART1_PORT_NUM, &uart_config));
	ESP_ERROR_CHECK(uart_set_pin(UART1_PORT_NUM, UART1_PIN_TXD, UART1_PIN_RXD, UART1_PIN_RTS, UART1_PIN_CTS));
#ifdef UART1_RS485
	ESP_ERROR_CHECK(uart_set_mode(UART1_PORT_NUM, UART_MODE_RS485_HALF_DUPLEX));
#endif

	xTaskCreate(Uart1EventTask, "uart1EventTask", 4096, NULL, 12, NULL);
}

/*
	ESP32 UART:
		https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/api-reference/peripherals/uart.html
		https://github.com/espressif/esp-idf/blob/36f49f361c001b49c538364056bc5d2d04c6f321/examples/peripherals/uart/uart_echo_rs485/main/rs485_example.c
		https://github.com/espressif/esp-idf/blob/master/examples/peripherals/uart/uart_echo/main/uart_echo_example_main.c
		https://github.com/espressif/esp-idf/blob/master/examples/peripherals/uart/uart_events/main/uart_events_example_main.c
		https://github.com/espressif/esp-idf/blob/master/examples/peripherals/uart/uart_async_rxtxtasks/main/uart_async_rxtxtasks_main.c
*/
