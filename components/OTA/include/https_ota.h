#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

void simple_ota_start(void);
esp_err_t fetch_version(char *version_buffer, int max_len);
#ifdef __cplusplus
}
#endif
