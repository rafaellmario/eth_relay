



#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "user_i2c.h"
#include "tca9555.h"
#include "user_mqtt.h"

static const char* TAG = "I2C";

static i2c_master_bus_handle_t i2c0BusHandler;
i2c_master_dev_handle_t i2c0_tca_output;
i2c_master_dev_handle_t i2c0_tca_input;

extern QueueHandle_t i2C_access_queue;
extern QueueHandle_t http_tca_out_get_queue; // Get input status
extern QueueHandle_t http_tca_inp_get_queue; // Get input status
extern QueueHandle_t mqtt_tca_exchange_queue;

// --------------------------------------------------------------------------------------------
//
static esp_err_t i2c_attach_device(uint16_t, i2c_master_bus_handle_t, i2c_master_dev_handle_t*);
static void      i2c_handle_task(void* pVParameters);

// --------------------------------------------------------------------------------------------
// 
esp_err_t user_i2c0_init()
{
    esp_err_t err = ESP_OK;
    i2c_master_bus_config_t i2cMasterCfg = 
    {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port   = I2C_NUM_0,
        .scl_io_num = I2C0_SCL_IO,
        .sda_io_num = I2C0_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true, 
    };

    err = i2c_new_master_bus(&i2cMasterCfg,&i2c0BusHandler);
    ESP_RETURN_ON_ERROR(err,TAG,"%s",esp_err_to_name(err));

    // Attach TCA outputs
    err = i2c_attach_device(TCA_ADDR_1,i2c0BusHandler,&i2c0_tca_output);
    ESP_RETURN_ON_ERROR(err,TAG,"%s",esp_err_to_name(err));

    // Attach TCA inputs
    err = i2c_attach_device(TCA_ADDR_2,i2c0BusHandler,
                                     &i2c0_tca_input);
    ESP_RETURN_ON_ERROR(err,TAG,"%s",esp_err_to_name(err));
    
    if(i2c_master_probe(i2c0BusHandler,TCA_ADDR_1,10) == ESP_OK && 
       i2c_master_probe(i2c0BusHandler,TCA_ADDR_2,10) == ESP_OK && 
       err == ESP_OK)
    {
        ESP_LOGI(TAG,"Devices at address 0x%x and 0x%x were detected.",TCA_ADDR_1,TCA_ADDR_2);
       
        // Create a queue to access the I2C bus
        i2C_access_queue = xQueueCreate(5,sizeof(i2c_access_ctrl_handle_t));

        // Create an task to control I2C access, pinned to core 1
        xTaskCreatePinnedToCore(i2c_handle_task,"I2CCtrl",
            configMINIMAL_STACK_SIZE+2048,
            NULL,5,NULL,1);
    }
    else
    {
        ESP_LOGE(TAG,"Failure to initializae I2C bus.");
        ESP_LOGE(TAG,"%s",esp_err_to_name(err));
    }

    return err;
}
// -----------------------------------------------------------------------------------------------
// 
// 
static esp_err_t i2c_attach_device(uint16_t device_address, i2c_master_bus_handle_t i2cBusHandle, 
    i2c_master_dev_handle_t* i2cDevHandle)
{

    esp_err_t err = ESP_OK;
    i2c_device_config_t i2cDevCfg = 
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = device_address,
        .scl_speed_hz    = I2C_FREQ_HZ
    };
    
    err = i2c_master_bus_add_device(i2cBusHandle,&i2cDevCfg,i2cDevHandle);

    return err;
}


// -------------------------------------------------------------------
// I2C Handle Task
static void i2c_handle_task(void* pVParameters)
{
    i2c_access_ctrl_handle_t i2c_access_handle;
    mqtt_access_ctrl_handle_t mqtt_pub_handle;
    
    uint16_t tca_output_status = 0x0000;
    uint16_t tca_input_status  = 0xFFFF;

    while(true)
    {
        xQueueReceive(i2C_access_queue,&i2c_access_handle,portMAX_DELAY);

        switch(i2c_access_handle.i2c_action)
        {
            case TCA_CFG_INPUT:
                tca_config_mode(i2c0_tca_input , 0xFFFF);
                break;
                
            case TCA_CFG_OUTPUT:
                tca_config_mode(i2c0_tca_output, 0x0000);
                break;

            
            case TCA_REFRESH_INP:
            case TCA_INTR_CHANGE:
                tca_input_status = tca_get(i2c0_tca_input); // Acess I2C device and get input status

                // Publish MQTT status on MQTT topic 
                if(mqtt_tca_exchange_queue != NULL)
                {
                    printf("Publishing input on topic\n");
                    mqtt_pub_handle.mqtt_action = MQTT_TCA_INP_PUB;
                    mqtt_pub_handle.tca_in_payload = tca_input_status;
                    xQueueSend(mqtt_tca_exchange_queue,&mqtt_pub_handle,pdMS_TO_TICKS(50));
                }
                
                break;

            case HTTP_TCA_INP_GET:
                xQueueSend(http_tca_inp_get_queue,&tca_input_status,pdMS_TO_TICKS(100));
                printf("Input status: %x\n",tca_input_status);
                break;

            case TCA_OUT_INIT:
                tca_set(i2c0_tca_output,0x0000); // Acess I2C device and set input status
                tca_output_status = 0x0000;      // Acess I2C device and get input status
                break;
            
            case MQTT_TCA_OUT_SET:
            case HTTP_TCA_OUT_SET:
                tca_set(i2c0_tca_output,i2c_access_handle.tca_out_stat); // Acess I2C device and set input status
                tca_output_status = tca_get(i2c0_tca_output);            // Acess I2C device and get input status

                // Publish MQTT status on MQTT topic 
                if(mqtt_tca_exchange_queue != NULL)
                {
                    printf("Publishing output on topic\n");
                    mqtt_pub_handle.mqtt_action = MQTT_TCA_OUT_PUB;
                    mqtt_pub_handle.tca_out_payload = tca_output_status;
                    xQueueSend(mqtt_tca_exchange_queue,&mqtt_pub_handle,pdMS_TO_TICKS(50));
                }
                break;

            case HTTP_TCA_OUT_GET:
                xQueueSend(http_tca_out_get_queue,&tca_output_status,pdMS_TO_TICKS(100));
                printf("Output status: %x\n",tca_output_status);
                break;
            
            case MQTT_TCA_INP_GET:
                mqtt_pub_handle.mqtt_action = MQTT_TCA_INP_PUB;
                mqtt_pub_handle.tca_in_payload = tca_input_status;
                xQueueSend(mqtt_tca_exchange_queue,&mqtt_pub_handle,pdMS_TO_TICKS(50));
                break;
            case MQTT_TCA_OUT_GET:
                mqtt_pub_handle.mqtt_action = MQTT_TCA_OUT_PUB;
                mqtt_pub_handle.tca_out_payload = tca_output_status;
                xQueueSend(mqtt_tca_exchange_queue,&mqtt_pub_handle,pdMS_TO_TICKS(50));
                break;
            default:
        };
    }
}






