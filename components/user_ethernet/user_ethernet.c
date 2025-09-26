

/*
 * User defined Ethernet library for WT32-ETH0
 * Autors: Rafael M. Silva (rsilva@lna.br)
 *         
 */

// ------------------------------------------------------
// Includes
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "driver/gpio.h"

#include "esp_log.h"
#include "esp_err.h"

#include "esp_eth.h"
#include "esp_system.h"

#include "esp_eth_driver.h"
#include "esp_check.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "lwip/ip4_addr.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "user_ethernet.h"

// ------------------------------------------------------
// Defines
#define ETHERNET_CONNECTED_BIT  BIT0
#define ETHERNET_FAIL_BIT       BIT1

// ------------------------------------------------------
// Global variables
static EventGroupHandle_t s_eth_event_group;
static const char* TAG = "ETH";

// ------------------------------------------------------
// Event handler for Ethernet events
static void eth_event_handler(void *arg, esp_event_base_t event_base, 
            int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};

    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) 
    {
        case ETHERNET_EVENT_CONNECTED:
            esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
            ESP_LOGI(TAG, "Ethernet Link Up"); // Ethernet got a valid link
            ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                    mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

            // xEventGroupClearBits(s_eth_event_group, ETHERNET_FAIL_BIT);
            // xEventGroupSetBits(s_eth_event_group,ETHERNET_CONNECTED_BIT);
            break;
        case ETHERNET_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "Ethernet Link Down"); // Ethernet lost a valid link   
            xEventGroupSetBits(s_eth_event_group, ETHERNET_FAIL_BIT);
            xEventGroupClearBits(s_eth_event_group,ETHERNET_CONNECTED_BIT);
            break;
        case ETHERNET_EVENT_START: // Ethernet driver start
            ESP_LOGI(TAG, "Ethernet driver start");
            break;
        case ETHERNET_EVENT_STOP: // Ethernet driver stop
            ESP_LOGW(TAG, "UNIDENTIFIED EVENT");
            break;
        default:
            break;
    }
}

// ------------------------------------------------------
// Event handler for IP_EVENT_ETH_GOT_IP
static void got_ip_event_handler(void *arg, esp_event_base_t event_base, 
            int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "IP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "Net Mask:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "Gateway:" IPSTR, IP2STR(&ip_info->gw));

    xEventGroupSetBits(s_eth_event_group, ETHERNET_CONNECTED_BIT);
    xEventGroupClearBits(s_eth_event_group,ETHERNET_FAIL_BIT);
}

// ------------------------------------------------------
// Settup the ESP32 internal EMAC layer
static esp_eth_handle_t eth_init_internal(esp_eth_mac_t **mac_out, esp_eth_phy_t **phy_out)
{
    // esp_eth_handle_t ret = NULL;
    // Init common MAC and PHY configs to default
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    mac_config.sw_reset_timeout_ms = 1000; // According to Arduino eth library

    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    // Update PHY config based on board specific configuration
    phy_config.phy_addr         = ETH_PHY_ADDR;
    phy_config.reset_gpio_num   = ETH_PHY_RST_GPIO;

    // Init vendor specific MAC config to default 
    eth_esp32_emac_config_t esp32_emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    esp32_emac_config.interface = EMAC_DATA_INTERFACE_RMII; // Reduced Media-Independent Interface (RMII)
    esp32_emac_config.clock_config.rmii.clock_mode = EMAC_CLK_EXT_IN;   // Esternal clock
    esp32_emac_config.clock_config.rmii.clock_gpio = EMAC_CLK_IN_GPIO;  // Emac clock input on GPIO0
    esp32_emac_config.smi_gpio.mdc_num   = ETH_MDC_GPIO; 
    esp32_emac_config.smi_gpio.mdio_num  = ETH_MDIO_GPIO;
    esp32_emac_config.dma_burst_len = ETH_DMA_BURST_LEN_4;

    // Create new ESP32 Ethernet MAC instance
    esp_eth_mac_t* mac = esp_eth_mac_new_esp32(&esp32_emac_config, &mac_config);
    if(mac == NULL)
        ESP_LOGE(TAG,"esp_eth_mac_new_esp32 failed");

    // Create new PHY instance based on board configuration -> LAN8720
    esp_eth_phy_t *phy = esp_eth_phy_new_lan87xx(&phy_config);
    if(phy == NULL)
        ESP_LOGE(TAG,"esp_eth_phy_new_lan87xx failed");

    // Enable external oscillator (pulled down at boot to allow IO0 strapping)
    ESP_ERROR_CHECK(gpio_set_direction(ETH_OSC_ENAB, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(ETH_OSC_ENAB, 1));
    ESP_LOGI(TAG, "Starting Ethernet interface...");

    // Init Ethernet driver to default and install it
    esp_eth_handle_t eth_handle = NULL;
    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);

    // ESP_GOTO_ON_FALSE(esp_eth_driver_install(&config, &eth_handle) == ESP_OK, NULL, err, TAG, "Ethernet driver install failed");
    if(esp_eth_driver_install(&config, &eth_handle) == ESP_OK)
    {
        if (mac_out != NULL)
            *mac_out = mac;
        if (phy_out != NULL)
            *phy_out = phy;
        return eth_handle;
    }
    else 
    {
        if (eth_handle != NULL) 
            esp_eth_driver_uninstall(eth_handle);
        if (mac != NULL)
            mac->del(mac);
        if (phy != NULL)
            phy->del(phy);
        return NULL;
    }
    
}


// ------------------------------------------------------
// user ethernet driver initialization
esp_err_t ethernet_setup(void)
{
    // esp_err_t err = ESP_OK;
    s_eth_event_group = xEventGroupCreate();

    // Initialize Ethernet driver
    esp_eth_handle_t eth_handle;
    eth_handle = eth_init_internal(NULL, NULL);
    if(eth_handle == NULL)
    {
        ESP_LOGE(TAG,"Failutre to create a EMAC layer!!");
        return ESP_FAIL;
    }
    
    // Initialize TCP/IP network interface (should be called only once in application)
    ESP_ERROR_CHECK(esp_netif_init());

    // Callback function to manage the ethernet events
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create instance(s) of esp-netif for Ethernet(s)
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t* eth_netif = esp_netif_new(&cfg);
    esp_netif_set_hostname(eth_netif,USER_ETH_HOSTNAME);
    
    #if STATIC_IP /* STATIC_IP */
        if (esp_netif_dhcpc_stop(eth_netif) != ESP_OK) 
        {
            ESP_LOGE(TAG, "Failed to stop dhcp client");
            return ESP_FAIL;
        }
        esp_netif_ip_info_t info_t;
        memset(&info_t, 0, sizeof(esp_netif_ip_info_t));
        ipaddr_aton((const char *)S_IP, &info_t.ip.addr);
        ipaddr_aton((const char *)GATEWAY, &info_t.gw.addr);
        ipaddr_aton((const char *)NETMASK, &info_t.netmask.addr);
        if(esp_netif_set_ip_info(eth_netif, &info_t) != ESP_OK)
            ESP_LOGE(TAG, "Failed to set ip info");
    #endif 

    // Attach Ethernet driver to TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));
    
    // Register user defined event handers for ethernet interface
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));
    
    // Start Ethernet driver state machine
    ESP_ERROR_CHECK(esp_eth_start(eth_handle));

    /*
     * Wait until the connection is stabilished (WIFI_CONNECTED_BIT) or
     * connection failure for the maximum number o re-tries (WIFI_FAIL_BIT) 
     */  
    EventBits_t bits = xEventGroupWaitBits(s_eth_event_group,
                                           ETHERNET_CONNECTED_BIT | ETHERNET_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    if (bits & ETHERNET_CONNECTED_BIT) 
    {
        ESP_LOGI(TAG, "Ethernet Connection established.\n");
        return ESP_OK;
    } 
    else if (bits & ETHERNET_FAIL_BIT)
        ESP_LOGE(TAG, "Ethernet Connection Failed.");
    else
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    return ESP_FAIL;
}


// ------------------------------------------------------
// Check for Ethernet connection status
bool user_eth_con_status(void)
{
    EventBits_t bits = xEventGroupGetBits(s_eth_event_group);
    if(bits&ETHERNET_CONNECTED_BIT)
        return true;
    else 
        return false;
}

// ------------------------------------------------
// EOF
// ------------------------------------------------