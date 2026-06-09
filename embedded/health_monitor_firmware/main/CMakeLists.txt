idf_component_register(SRCS "health_monitor_firmware.c"
                       PRIV_REQUIRES spi_flash driver bt nvs_flash
                       INCLUDE_DIRS "")