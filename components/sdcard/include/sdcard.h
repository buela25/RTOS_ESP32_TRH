#pragma once

#include "esp_err.h"

esp_err_t sdcard_init(void);
esp_err_t sdcard_log_th(float temperature, float humidity);
void sdcard_deinit(void);
