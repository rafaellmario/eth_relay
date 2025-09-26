

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "tca9555.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "driver/i2c_master.h"
#include "driver/gpio.h"

#include "esp_log.h"
#include "esp_err.h"

#include "user_i2c.h"

extern i2c_master_dev_handle_t i2c0_tca_output;
extern i2c_master_dev_handle_t i2c0_tca_input;
extern QueueHandle_t i2C_access_queue;

static const char* TAG = "TCA9555";

// -------------------------------------------------------------------
// External interruption handle
static void IRAM_ATTR tca_change_isr_handler(void* PvParameters)   
{
   i2c_access_ctrl_handle_t tca_change_int;
   tca_change_int.i2c_action = TCA_INTR_CHANGE;

   BaseType_t xHigherPriorityTaskWoken = pdTRUE;
   xQueueSendFromISR(i2C_access_queue,
        &tca_change_int,
        &xHigherPriorityTaskWoken);

    //check if the interrupt raised the task priorit
    if(xHigherPriorityTaskWoken)
        portYIELD_FROM_ISR (); //force context switch

}


// -------------------------------------------------------------------
// TCA9555 Configuration 
// - device: Device handler for communitation with
// - bits: 16 bits register where each bit control TCA IO operation mode
// -- 1: for input
// -- 0: for output
esp_err_t tca_config_mode(i2c_master_dev_handle_t device, uint16_t bits)
{
    // Send most significant bits
    esp_err_t err = ESP_OK;
    uint8_t reg[] = {0, 0, 0};

    reg[0] = TCA9555_CONFIG_PORT0;
    reg[1] = ((0xFF00 & bits) >> 8); // 8 most significant bits
    reg[2] = ((0x00FF & bits)); // 8 less significant bits
    err = i2c_master_transmit(device,reg,3,50);

    return err;
}

// -------------------------------------------------------------------
// TCA9555 Set 
// - device: Device handler for communitation with
// - bits: 16 bits register where each bit control the output level
// -- 1: Set for high level
// -- 0: Clear pin to low level
void tca_set(i2c_master_dev_handle_t device, uint16_t bits)
{
    uint8_t reg[] = {0, 0, 0};
    reg[0] = TCA9555_OUT_PORT0;
    reg[1] = (uint8_t)(0x00FF & bits);
    reg[2] = (uint8_t)((0xFF00 & bits) >> 8);
    ESP_ERROR_CHECK(i2c_master_transmit(device,reg,3,50));

    return;
}

// -------------------------------------------------------------------
// Get TCA pins states
// - device: Device handler for communitation with
// - bits: 16 bits register where each bit control the output level
// -- 1: Set for low level
// -- 0: Keep the output in previous state
uint16_t tca_get(i2c_master_dev_handle_t device)
{
    uint8_t  address =  TCA9555_IN_PORT0;
    uint8_t  read_ports[] = {0,0};
    uint16_t bits = 0;
    // Read the port actual status
    // TCA9555_IN_PORTx reflects the incoming logic levels of the pins,
    // regardless of whether the pin is IN or OUT
    ESP_ERROR_CHECK(i2c_master_transmit_receive(device,&address,1,read_ports,2,50));

    bits = (0xFF & read_ports[1]) << 8;   // Get MSBs
    bits = bits | (0xFF & read_ports[0]); // Get LSBs

    return bits;
}

// -------------------------------------------------------------------
// TCA initialization 
esp_err_t tca9555_init()
{
    esp_err_t err  = ESP_OK;
    // -----------------------------------------------------------
    // Configure a pin for TCA input port change interruption
    gpio_config_t ioConfig = 
    {
        .pin_bit_mask = (1ULL << TCA9555_INTR_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    
    err = gpio_config(&ioConfig);

    err = gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);

    // Create an interruption service routine to handle the port change interruption
    err = gpio_isr_handler_add(TCA9555_INTR_PIN,tca_change_isr_handler,NULL);
    
    // Config TCA1 as input buffer
    err = tca_config_mode(i2c0_tca_output, 0x0000);  // Define all pins as outputs
    tca_set(i2c0_tca_output,0x0000); // Set all pins to zero 

    // Config TCA2 as output buffer
    err = tca_config_mode(i2c0_tca_input , 0xFFFF);  // Define all pins as inputs
    
    return err;
}