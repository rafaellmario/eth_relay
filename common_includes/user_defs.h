

#ifndef USER_DEFS_H
#define USER_DEFS_H

#include "stdbool.h"
#include "stdint.h"

struct failure_check_t
{

    bool i2c_failure:1;
    bool tca_failure:1;
    bool eth_failure:1;
    bool mqtt_failure:1;
    bool http_failure:1;
    bool nvs_failure:1;
    bool init_failure:1;
    uint16_t RSV:11;
};


typedef union failures_reg_t
{
    uint16_t all;
    struct failure_check_t bit;
} failure_chain_t;


#endif