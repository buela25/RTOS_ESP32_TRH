#include "https_ota.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "esp_http_client.h"

#include "nvs_flash.h"

#ifdef CONFIG_EXAMPLE_USE_CERT_BUNDLE
#include "esp_crt_bundle.h"
#endif

static const char *TAG = "simple_ota";

/* ---- GitLab OTA configuration ---- */
#define PROJECT_PATH "trh%2Ftrh_temperature_andrh-datalogger"
#define FILE_PATH    "OTA%2FTRH_ESPRESSIF.bin"
#define VERSION_PATH    "OTA%2Fversion.txt"
#define BRANCH       "feature-test"
#define GITLAB_PAT   "xxxxx-xxxxx_xxxxxxxxxxxxxx"

#define OTA_URL "https://scm.ckdigital.in/api/v4/projects/" PROJECT_PATH "/repository/files/" FILE_PATH "/raw?ref=" BRANCH
#define OTA_URL_V "https://scm.ckdigital.in/api/v4/projects/" PROJECT_PATH "/repository/files/" VERSION_PATH "/raw?ref=" BRANCH

/* ---- HTTP event handler ---- */
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_CONNECTED) {
        esp_http_client_set_header(evt->client, "PRIVATE-TOKEN", GITLAB_PAT);
    }
    return ESP_OK;
}

esp_err_t fetch_version(char *version, int max_len)
{
    esp_http_client_config_t config = {
    .url = OTA_URL_V,
    .event_handler = http_event_handler,
    .method = HTTP_METHOD_GET,
    .timeout_ms = 5000,
    .crt_bundle_attach = esp_crt_bundle_attach,  
};

    // esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_handle_t client = esp_http_client_init(&config);

    if (esp_http_client_open(client, 0) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection");
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    esp_http_client_fetch_headers(client);

    int read_len = esp_http_client_read(client, version, max_len - 1);

    if (read_len <= 0) {
        ESP_LOGE(TAG, "Failed to read version");
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    version[read_len] = '\0';

    // remove newline if present
    char *newline = strchr(version, '\n');
    if (newline) *newline = '\0';

    ESP_LOGI(TAG, "Server version: %s", version);

    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    return ESP_OK;
}


/* ---- OTA task ---- */
static void ota_task(void *pv)
{
    ESP_LOGI(TAG, "Starting OTA...");

    esp_http_client_config_t http_cfg = {
        .url = OTA_URL,
        .event_handler = http_event_handler,
#ifdef CONFIG_EXAMPLE_USE_CERT_BUNDLE
        .crt_bundle_attach = esp_crt_bundle_attach,
#endif
        .timeout_ms = 10000,
        .keep_alive_enable = false,
    };

    esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
    };

    esp_err_t ret = esp_https_ota(&ota_cfg);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA success, rebooting");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "OTA failed: %s", esp_err_to_name(ret));
    }

    vTaskDelete(NULL);
}

/* ---- Public API ---- */
void simple_ota_start(void)
{
    xTaskCreate(ota_task, "ota_task", 8192, NULL, 5, NULL);
}
