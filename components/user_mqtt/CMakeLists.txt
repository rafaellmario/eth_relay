idf_component_register(SRCS "user_mqtt.c"
                    INCLUDE_DIRS "include"
                    REQUIRES
                    "mqtt"
                    "user_i2c")

