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

#include "driver/gpio.h"
#include "driver/i2c_master.h"

#include "user_i2c.h"
#include "tca9555.h"

#include "user_ethernet.h"

#include "user_defs.h"

QueueHandle_t i2C_access_queue;
static const char* TAG = "MAIN";

failure_chain_t check_chain;

void app_main(void)
{
    // ----------------------------------------------
    // NVS initialization
    esp_err_t err = nvs_flash_init();
    if(err == ESP_ERR_NVS_NO_FREE_PAGES || 
       err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    check_chain.all = false;

    // I2C Bus initialization
    ESP_LOGI(TAG,"I2C initialization.");
    ESP_ERROR_CHECK(user_i2c0_init());
    // TCA initialization 
    ESP_LOGI(TAG,"I2C initialization.");
    ESP_ERROR_CHECK(tca9555_init());

    // Ethernet initialization
    ESP_ERROR_CHECK(ethernet_setup());
    
    while(true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ----------------------------------------------------------------------------------
// I2C Bus attach a device





