idf_component_register(SRCS "user_ethernet.c"
                    INCLUDE_DIRS "include"
                    REQUIRES
                    "esp_driver_gpio" "esp_eth" "esp_netif")
