idf_component_register(
    SRCS "https_ota.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_https_ota app_update esp_http_client nvs_flash mbedtls
)
