/* https://github.com/espressif/esp-idf/blob/master/examples/ethernet/basic/main/ethernet_example_main.c

GPIO32 - PHY_POWER   : NC - Osc. Enable - 4k7 Pulldown

GPIO19 - EMAC_TXD0   : TX0
GPIO22 - EMAC_TXD1   : TX1
GPIO21 - EMAC_TX_EN  : TX_EN
GPIO25 - EMAC_RXD0   : RX0
GPIO26 - EMAC_RXD1   : RX1
GPIO27 - EMAC_RX_DV  : CRS
GPIO00 - EMAC_TX_CLK : nINT/REFCLK (50MHz) - 4k7 Pullup
GPIO23 - SMI_MDC     : MDC
GPIO18 - SMI_MDIO    : MDIO
GND                  : GND
3V3                  : VCC

Because of its freqency the signal integrity has to be observed
(ringing, capacitive load, resisitive load, skew, length of PCB trace).
It is recommended to add a 33Ω resistor in series to reduce ringing.
*/

#include "../../include/main.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"

const char *TAG_LAN8720A = "Lan8720a";

#define CONFIG_ETH_PHY_ADDR       0			// RXERR/PHYAD0
#define CONFIG_ETH_PHY_POWER_GPIO 32
#define CONFIG_ETH_PHY_RST_GPIO   -1
#define CONFIG_ETH_MDC_GPIO       23
#define CONFIG_ETH_MDIO_GPIO      18

void Lan8720a_EthEventHandler(void *arg, esp_event_base_t event_base,
		int32_t event_id, void *event_data) __attribute__((weak));

/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
		int32_t event_id, void *event_data)
{
	uint8_t macAddr[6] = {0};
		/* we can get the ethernet driver handle from event data */
	esp_eth_handle_t ethHandle = *(esp_eth_handle_t *)event_data;

	switch (event_id)
	{
		case ETHERNET_EVENT_CONNECTED:
			esp_eth_ioctl(ethHandle, ETH_CMD_G_MAC_ADDR, macAddr);
			ESP_LOGI(TAG_LAN8720A, "Ethernet Link Up");
			ESP_LOGI(TAG_LAN8720A, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
					macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
			break;
		case ETHERNET_EVENT_DISCONNECTED:
			ESP_LOGI(TAG_LAN8720A, "Ethernet Link Down");
			break;
		case ETHERNET_EVENT_START:
			ESP_LOGI(TAG_LAN8720A, "Ethernet Started");
			break;
		case ETHERNET_EVENT_STOP:
			ESP_LOGI(TAG_LAN8720A, "Ethernet Stopped");
			break;
		default:
			break;
	}

	Lan8720a_EthEventHandler(arg, event_base, event_id, event_data);
}

void Lan8720a_GotIpEventHandler(void *arg, esp_event_base_t event_base,
		int32_t event_id, void *event_data) __attribute__((weak));

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
		int32_t event_id, void *event_data)
{
	ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
	const esp_netif_ip_info_t *ip_info = &event->ip_info;

	ESP_LOGI(TAG_LAN8720A, "Ethernet Got IP Address");
	ESP_LOGI(TAG_LAN8720A, "  ip: " IPSTR ", mask: " IPSTR ", gw: " IPSTR, IP2STR(&ip_info->ip),
			IP2STR(&ip_info->netmask), IP2STR(&ip_info->gw));

	Lan8720a_GotIpEventHandler(arg, event_base, event_id, event_data);
}

void Lan8720a_Initialize(void)
{
		// Set LAN8720a power on
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = 1ULL << CONFIG_ETH_PHY_POWER_GPIO;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpio_config(&io_conf);
	gpio_set_level(CONFIG_ETH_PHY_POWER_GPIO, 1);
	DELAY_MS(120);

		// Initialize TCP/IP network interface (should be called only once in application)
	ESP_ERROR_CHECK(esp_netif_init());
		// Create default event loop that running in background
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
	esp_netif_t *eth_netif = esp_netif_new(&cfg);
		// Set default handlers to process TCP/IP stuffs
		// deprecated https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32s3/migration-guides/ethernet.html
	// ESP_ERROR_CHECK(esp_eth_set_default_handlers(eth_netif));
		// Register user defined event handers
	ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

	eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
	eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
	phy_config.phy_addr = CONFIG_ETH_PHY_ADDR;
	phy_config.reset_gpio_num = CONFIG_ETH_PHY_RST_GPIO;
	mac_config.smi_mdc_gpio_num = CONFIG_ETH_MDC_GPIO;
	mac_config.smi_mdio_gpio_num = CONFIG_ETH_MDIO_GPIO;
	esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&mac_config);
	esp_eth_phy_t *phy = esp_eth_phy_new_lan8720(&phy_config);
	esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
	esp_eth_handle_t ethHandle = NULL;
	ESP_ERROR_CHECK(esp_eth_driver_install(&config, &ethHandle));

		/* attach Ethernet driver to TCP/IP stack */
	ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(ethHandle)));
		/* start Ethernet driver state machine */
	ESP_ERROR_CHECK(esp_eth_start(ethHandle));
}
