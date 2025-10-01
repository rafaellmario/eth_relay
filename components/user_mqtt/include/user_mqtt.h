
#ifndef  USER_MQTT_H
#define  USER_MQTT_H


#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#define ESP_BROKER_URL "mqtt://192.168.2.101"
#define ESP_BROKER_PORT 1883

#define RELAY_INPUT_GET   "relay/input/get"
#define RELAY_INPUT_PUB   "relay/input/pub"

#define RELAY_OUTPUT_SET  "relay/output/set"
#define RELAY_OUTPUT_GET  "relay/output/get"
#define RELAY_OUTPUT_PUB  "relay/output/pub"



#define RELAY_STATUS "relay/status"


typedef enum 
{
    MQTT_TCA_INP_PUB,
    MQTT_TCA_OUT_PUB
} mqtt_action_type_h;


typedef struct mqtt_access_ctrl_t
{
    uint16_t tca_in_payload;
    uint16_t tca_out_payload;
    mqtt_action_type_h mqtt_action;
} mqtt_access_ctrl_handle_t;


esp_err_t user_mqtt_start(void);
void user_mqtt_subscribe(char* topic, int qos);
void user_mqtt_unsubscribe(char* topic);
void user_mqtt_publish(char* topic, char* payload, int qos, bool retain);
bool user_mqtt_con_status(void);
void user_mqtt_stop(void);

#endif