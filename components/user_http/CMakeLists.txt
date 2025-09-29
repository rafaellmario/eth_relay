idf_component_register(SRCS "user_http.c"
                    INCLUDE_DIRS "include"
                    REQUIRES 
                    "esp_http_server"
                    "user_i2c")

