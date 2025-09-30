/*
 * Remote relay:
 * Project for load remote control using 
 * WT32-ETH0 and MQTT
 * Authors: Rafael M. Silva (rsilva@lna.br)
 *         
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "nvs_flash.h"

// #include "driver/gpio.h"
// #include "driver/i2c_master.h"

// User defined libraries
#include "user_i2c.h"
#include "tca9555.h"
#include "user_ethernet.h"
#include "user_http.h"
#include "user_defs.h"

QueueHandle_t i2C_access_queue;        // Access control to i2c bus
QueueHandle_t http_tca_out_get_queue;  // Http get output status
QueueHandle_t http_tca_inp_get_queue;  // Http get input status
QueueHandle_t mqtt_tca_exchange_queue; // data exchange between MQTT and tca expansions

static const char* TAG = "MAIN";

failure_chain_t check_chain;

void app_main(void)
{
    esp_err_t err = ESP_OK;
    check_chain.all = false;

    // ----------------------------------------------
    // NVS initialization
    err = nvs_flash_init();
    if(err == ESP_ERR_NVS_NO_FREE_PAGES || 
       err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    if(err != ESP_OK)
    {
        check_chain.bit.nvs_failure = true; // There is an error on NVS initialization
        ESP_LOGE(TAG,"%s",esp_err_to_name(err));
    }
    
    // ----------------------------------------------
    // I2C Bus initialization
    if(check_chain.all == 0)
    {
        ESP_LOGI(TAG,"I2C initialization.");
        err = user_i2c0_init();

        if(err != ESP_OK)
        {
            check_chain.bit.i2c_failure = true;     // There is an error on I2C initialization
            ESP_LOGE(TAG,"%s",esp_err_to_name(err));
        }
    }
    
    // ----------------------------------------------
    // TCA initialization 
    if(check_chain.all == 0)
    {
        ESP_LOGI(TAG,"TCA initialization.");
        err = tca9555_init();
        if(err != ESP_OK)
        {
            check_chain.bit.tca_failure = true; // There is an error on tca initialization
            ESP_LOGE(TAG,"%s",esp_err_to_name(err));
        }
    }
    
    // ----------------------------------------------
    // Ethernet initialization
    if(check_chain.all == 0)
    {
        ESP_LOGI(TAG,"Ethernet initialization.");
        err = ethernet_setup();
        if(err != ESP_OK)
        {
            check_chain.bit.eth_failure = true; // There is an error on ethernet initialization
            ESP_LOGE(TAG,"%s",esp_err_to_name(err));
        }
    }
    
    // ----------------------------------------------
    // Web server initialization
    if((check_chain.all == 0) && user_eth_con_status())
    {
        start_webserver( );
    }


    while(true)
    {

        if(check_chain.all != 0)
            ESP_LOGE(TAG,"Device initialization failure ");

           
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ----------------------------------------------------------------------------------
// I2C Bus attach a device





