idf_component_register(SRCS "harmony_soc_main.c"
                            "app_handler/app_handler.c"
                            "app_handler/app_handler_ble.c"
                            "app_handler/app_handler_uart.c"
                    INCLUDE_DIRS "."
                                "app_handler"
                    PRIV_REQUIRES  hal_platform hal_uart hal_wifi hal_ble  third_libs hal_ezos)
