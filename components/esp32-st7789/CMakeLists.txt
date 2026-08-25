idf_component_register(SRCS "spi_bridge.c" "driver_st7789.c"
                       INCLUDE_DIRS "include"
                       REQUIRES freertos esp_driver_spi esp_driver_gpio
                       PRIV_REQUIRES esp_system log)
