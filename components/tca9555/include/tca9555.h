
#ifndef TCA9555_H
#define TCA9555_H


#include "esp_err.h"
#include "driver/i2c_master.h"

#define TCA9555_INTR_PIN GPIO_NUM_14

// ----------------------------------------------------------------------------------
// Register map
#define TCA9555_IN_PORT0      0x00 // Input Port 0 Read byte
#define TCA9555_IN_PORT1      0x01 // Input Port 1 Read byte
#define TCA9555_OUT_PORT0     0x02 // Output Port 0 Read-write byte
#define TCA9555_OUT_PORT1     0x03 // Output Port 1 Read-write byte
#define TCA9555_POL_INV_PORT0 0x04 // Polarity Inversion Port 0 Read-write byte
#define TCA9555_POL_INV_PORT1 0x05 // Polarity Inversion Port 1 Read-write byte

// If a bit is set to 1, the corresponding port pin is an input 
// If a bit is cleared to 0, the corresponding port pin is an output.
#define TCA9555_CONFIG_PORT0 0x06 // Configuration Port 0 Read-write byte
#define TCA9555_CONFIG_PORT1 0x07 // Configuration Port 1 Read-write byte


esp_err_t tca9555_init();
esp_err_t tca_config_mode(i2c_master_dev_handle_t device, uint16_t bits);
void tca_set(i2c_master_dev_handle_t device, uint16_t bits);
uint16_t tca_get(i2c_master_dev_handle_t device);


#endif