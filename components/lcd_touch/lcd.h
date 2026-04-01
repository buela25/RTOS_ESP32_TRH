#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char ip[32];
    char ap[40];
    char mac[32];
    char device_id[32];
    char temp[10];
    char humidity[10];
} lcd_wifi_info_t;

void initialize_display(void);
void lcd_post_wifi_info(const lcd_wifi_info_t *info);
#ifdef __cplusplus
}
#endif
