idf_component_register(
    SRCS
        "lcd.c"
        "ui.c"
        "ui_screen_1.c"
        "ui_screen_2.c"
        "ui_screen_3.c"
        "ui_comp_hook.c"
        "ui_helpers.c"
        "ui_events.c"
        # "ui_img_1686735119.c"
        "ui_img_1794831165.c"
        "ui_img_info_icon_png.c"
        "ui_img_disconnect_png.c"
        "ui_img_logo_png.c"
        # "ui_temporary_image.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        esp_timer
        esp_lcd
        driver
        lvgl
        freertos
        espressif__esp_lcd_ili9341
        atanisoft__esp_lcd_touch_xpt2046
)
