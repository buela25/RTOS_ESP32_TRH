#include "influxdb.h"
#include "esp_http_client.h"
#include "esp_log.h"

#define INFLUX_URL  "http://34.195.134.255:8086/api/v2/write?bucket=TRH_R%26D&org=a650b061a7b3193e&precision=s"
#define INFLUX_TOKEN "ofQbGQ_5WqNsnBJI0_TXHypnMMW1oXWf6bvHMYwNG5T4OJ7WhCGTQz07UDROmeHIjSQRDzhndc2yqRV2e00_6g=="

static esp_http_client_handle_t client;
static const char *TAG = "INFLUX";
static char device_id_str[32];

void influx_init(const char* device_id)
{
    ESP_LOGI("INFLUX", "init_devid: %s", device_id);
    snprintf(device_id_str, sizeof(device_id_str), "%s", device_id);
    esp_http_client_config_t config = {
        .url = INFLUX_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
        .keep_alive_enable = true
    };

    client = esp_http_client_init(&config);

    esp_http_client_set_header(client, "Authorization", "Token " INFLUX_TOKEN);
    esp_http_client_set_header(client, "Content-Type", "text/plain");
}

void influx_write(float temperature, float humidity)
{
    char payload[256];

    snprintf(payload, sizeof(payload),
             "%s temperature=%.2f,humidity=%.2f",
             device_id_str,
             temperature,
             humidity);

    esp_http_client_set_post_field(client, payload, strlen(payload));
    ESP_LOGI("INFLUX", "Payload: %s", payload);
    esp_err_t err = esp_http_client_perform(client);

    int status = esp_http_client_get_status_code(client);

    if (err == ESP_OK && status == 204) {
        ESP_LOGI(TAG, "Write successful");
    } else {
        ESP_LOGE(TAG, "Write failed: %d", status);
    }
}

void influx_deinit(void)
{
    esp_http_client_cleanup(client);
}