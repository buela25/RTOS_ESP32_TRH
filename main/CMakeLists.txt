idf_component_register(
    SRCS "main.c"
    REQUIRES aht20 sdcard OTA lcd_touch esp32-wifi-manager influxdb driver  
)
