#ifndef USER_I2C_H
#define USER_I2C_H

#include "esp_err.h"

#define I2C0_SCL_IO GPIO_NUM_17     // GPIO number used for I2C master clock
#define I2C0_SDA_IO GPIO_NUM_5      // GPIO number used for I2C master data 
#define I2C_FREQ_HZ 400000          // I2C master clock frequency
#define I2C0_TX_BUFFER_DISABLE 0    // I2C master doesn't need buffer 
#define I2C0_RX_BUFFER_DISABLE 0    // I2C master doesn't need buffer 

#define TCA_ADDR_1 0x20 // I2C device address
#define TCA_ADDR_2 0x27 // I2C device address


// -----------------------------------------------------
// i2c device data exchange struct
typedef enum 
{   
    TCA_CFG_INPUT,
    TCA_CFG_OUTPUT,
    TCA_INTR_CHANGE,  // TCA input change interruption
    TCA_OUT_INIT,
    TCA_REFRESH_INP,
    HTTP_TCA_INP_GET,     // HTTP input status request 
    HTTP_TCA_OUT_SET,     // HTTP output set pins 
    HTTP_TCA_OUT_GET      // HTTP output status request
} i2c_action_type_t;

typedef struct i2c_access_ctrl_t
{
    uint16_t tca_in_stat;
    uint16_t tca_out_stat;
    i2c_action_type_t i2c_action;
} i2c_access_ctrl_handle_t;


// -----------------------------------------------------
esp_err_t user_i2c0_init();


#endif