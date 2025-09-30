

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"

#include "esp_http_server.h"
#include "user_http.h"
#include "user_i2c.h"

static const char* TAG = "HTTP SERVER";
static bool outputs[16] = {0};
static bool inputs[16]  = {0};

extern QueueHandle_t http_tca_out_get_queue; // Get input status
extern QueueHandle_t http_tca_inp_get_queue; // Get input status
extern QueueHandle_t i2C_access_queue;  // Write and reqeuest on i2C bus


// HTML main page
static const char index_html[] =
"<!DOCTYPE html>"
"<html>"
"<head>"
  "<meta charset='utf-8'>"
  "<title>Output status</title>"
  "<style>"
    "body {"
      "font-family: Arial, sans-serif;"
      "text-align: center;"
      "margin: 0;"
      "padding: 20px;"
      "background: #f4f6f8;"
      "color: #333;"
    "}"
    "h2 {"
      "margin-bottom: 20px;"
    "}"
    ".grid {"
      "display: grid;"
      "grid-template-columns: repeat(8, 70px);"
      "grid-gap: 15px;"
      "justify-content: center;"
      "margin-bottom: 20px;"
    "}"
    ".btn {"
      "width: 60px; height: 60px;"
      "border-radius: 50%;"
      "border: none;"
      "cursor: pointer;"
      "color: #fff;"
      "font-size: 14px;"
      "font-weight: bold;"
      "box-shadow: 0 3px 6px rgba(0,0,0,0.2);"
      "transition: transform 0.15s, box-shadow 0.2s;"
    "}"
    ".btn:active {"
      "transform: scale(0.92);"
      "box-shadow: 0 2px 4px rgba(0,0,0,0.2);"
    "}"
    ".on {"
      "background: linear-gradient(145deg, #28a745, #218838);"
    "}"
    ".off {"
      "background: linear-gradient(145deg, #dc3545, #c82333);"
    "}"
  "</style>"
"</head>"
"<body>"
  "<h2>Remote Relay WEB Control</h2>"
  "<h3>Outputs</h3>"
"<div class='grid' id='row_out1'></div>"
"<div class='grid' id='row_out2'></div>"
"<h3>Inputs</h3>"
"<div class='grid' id='row_in1'></div>"
"<div class='grid' id='row_in2'></div>"
"<script>"

"function createButtons() {"
  "for (let i = 0; i < 16; i++) {"
   "let btnOut = document.createElement('button');"
    "btnOut.id = 'out'+i;"
    "btnOut.className = 'btn off';"
    "btnOut.innerText = i;"
    "btnOut.onclick = () => toggle(i);"
    "if (i < 8) document.getElementById('row_out1').appendChild(btnOut);"
    "else document.getElementById('row_out2').appendChild(btnOut);"
    "let btnIn = document.createElement('button');"
    "btnIn.id = 'in'+i;"
    "btnIn.className = 'btn off';"
    "btnIn.innerText = i;"
    "btnIn.disabled = true;"
    "if (i < 8) document.getElementById('row_in1').appendChild(btnIn);"
    "else document.getElementById('row_in2').appendChild(btnIn);"
  "}"
"}"

"function toggle(pin) {"
    "fetch(`/toggle?pin=${pin}`)"
      ".then(r => r.json())"
     ".then(data => updateUI(data));"
 "}"

"function updateUI(states) {"
  "for (let pin in states.outputs) {"
    "let btn = document.getElementById('out'+pin);"
    "if (states.outputs[pin] == 1) {"
      "btn.classList.remove('off'); btn.classList.add('on');"
    "} else {"
      "btn.classList.remove('on'); btn.classList.add('off');"
    "}"
  "}"
  "for (let pin in states.inputs) {"
    "let btn = document.getElementById('in'+pin);"
    "if (states.inputs[pin] == 1) {"
      "btn.classList.remove('off'); btn.classList.add('on');"
    "} else {"
      "btn.classList.remove('on'); btn.classList.add('off');"
    "}"
  "}"
"}"
    "function refresh() {"
      "fetch('/status')"
      ".then(r => r.json())"
      ".then(data => updateUI(data));"
    "}"
    "createButtons();"
    "setInterval(refresh, 1000);"
    "refresh();"
  "</script>"
"</body>"
"</html>";

// ------------------------------------------------
// Handler of initial (index) page
static esp_err_t index_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}


// ------------------------------------------------
// Handler of /status → returning a JSON
// Status page handler, the webpage buttons status are 
// updated according to the output states 
static esp_err_t status_handler(httpd_req_t *req)
{
    char buffer[256];
    int offset = 0;
    // uint16_t out_data = 0, inp_data = 0;
    i2c_access_ctrl_handle_t i2c_access_handle;

    // output queue handle here
    i2c_access_handle.i2c_action = HTTP_TCA_OUT_GET; // send a requisition of output status
    xQueueSend(i2C_access_queue,&i2c_access_handle,pdMS_TO_TICKS(50));
    xQueueReceive(http_tca_out_get_queue,&(i2c_access_handle.tca_out_stat),pdMS_TO_TICKS(100));

    // i2c_access_handle.tca_out_stat = 0x0000;
    ESP_LOGI(TAG,"Received output: %d", i2c_access_handle.tca_out_stat); 

    // Trasnfer output data to array
    for (int i = 0; i < 16; i++) 
        outputs[i] = (bool)(((i2c_access_handle.tca_out_stat) >> i) & 0x0001);

    // Data exchange with I2C
    i2c_access_handle.i2c_action = HTTP_TCA_INP_GET; // send a requisition of input status
    xQueueSend(i2C_access_queue,&i2c_access_handle,pdMS_TO_TICKS(50));
    xQueueReceive(http_tca_inp_get_queue,&(i2c_access_handle.tca_in_stat),pdMS_TO_TICKS(100));

    // i2c_access_handle.tca_in_stat = 0x0000;
    ESP_LOGI(TAG,"Received input: %d", i2c_access_handle.tca_in_stat);       
    // Transfer input data to array 
    for (int i = 0; i < 16; i++)
        inputs[i] = (bool)(((i2c_access_handle.tca_in_stat) >> i) & 0x0001);
    
    
    // Create a JSON string 
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "{");

    // outputs
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\"outputs\":{");
    for (int i = 0; i < 16; i++) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset,
            "\"%d\":%d%s", i, outputs[i], (i < 15 ? "," : ""));
    }
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "},");

    // inputs
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\"inputs\":{");
    for (int i = 0; i < 16; i++) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset,
            "\"%d\":%d%s", i, inputs[i], (i < 15 ? "," : ""));
    }
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "}");

    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "}");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buffer, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}


// ------------------------------------------------
// Handler of /toggle?pin=X
// change the button status on WEB page click
static esp_err_t toggle_handler(httpd_req_t *req)
{
    char query[32];
    uint16_t relayData = 0;
    i2c_access_ctrl_handle_t i2c_access_handle;

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) 
    {
        char param[8];
        if (httpd_query_key_value(query, "pin", param, sizeof(param)) == ESP_OK) 
        {
            int pin = atoi(param);
            if (pin >= 0 && pin < 16) 
            {
                outputs[pin] = !outputs[pin]; // 
                ESP_LOGI(TAG, "Toggle pin %d -> %d", pin, outputs[pin]);
                // Queue to write the value on TCA9555 
                for(int i = 0; i< 16; i++)
                    relayData = relayData | (outputs[i] << i);
                printf("HTTP Toggle: 0x%x\n",relayData);

                i2c_access_handle.i2c_action = HTTP_TCA_OUT_SET;
                i2c_access_handle.tca_out_stat = relayData;
                xQueueSendToFront(i2C_access_queue,&i2c_access_handle,pdMS_TO_TICKS(50)); // Set TCA output 
            }
        }
    }

    char buffer[256];
    int offset = 0;
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "{");
    for (int i = 0; i < 16; i++) 
    {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                           "\"%d\":%d%s", i, outputs[i], (i < 15 ? "," : ""));
    }
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "}");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buffer, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

// ------------------------------------------------
httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) 
    {
        httpd_uri_t uri_index = 
        {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = index_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &uri_index);

        httpd_uri_t uri_status = 
        {
            .uri       = "/status",
            .method    = HTTP_GET,
            .handler   = status_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &uri_status);

        httpd_uri_t uri_toggle = 
        {
            .uri       = "/toggle",
            .method    = HTTP_GET,
            .handler   = toggle_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &uri_toggle);

        ESP_LOGI(TAG, "Start web server");

        // Create queues for data exchange with i2c
        http_tca_out_get_queue = xQueueCreate(1,sizeof(uint16_t));
        http_tca_inp_get_queue = xQueueCreate(1,sizeof(uint16_t));
    }

    return server;
}







