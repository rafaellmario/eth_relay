

#ifndef USER_HTTP_H
#define USER_HTTP_H

#include "esp_http_server.h"

httpd_handle_t start_webserver(bool system_failure,char msg[]);


#endif

