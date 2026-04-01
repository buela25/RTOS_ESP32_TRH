#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2c.h"
#include "aht20.h"
#include "sdcard.h"
#include "https_ota.h"
#include "lcd.h"
#include "esp_log.h"
#include "esp_system.h"
#include <esp_wifi.h>
#include <esp_netif.h>
#include "esp_mac.h"
#include "esp_app_desc.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "ui.h"
#include "wifi_manager.h"
#include "influxdb.h"

#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_SDA        22
#define I2C_SCL        21


static const char TAG[] = "main";
extern lcd_wifi_info_t info;
aht20_data_t data;
QueueHandle_t influx_queue;

char device_id[32];

static void i2c_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };

    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

void monitoring_task(void *pvParameter)
{
	for(;;){
		ESP_LOGI(TAG, "free heap: %d",esp_get_free_heap_size());
		vTaskDelay( pdMS_TO_TICKS(10000) );
	}
}

static void sensor_task(void *arg)
{

    while (1) {
        if (aht20_read(I2C_MASTER_NUM, &data) == ESP_OK) {
            printf("Temp: %.2f C, Hum: %.2f %%\n",
                   data.temperature, data.humidity);
            
            xQueueSend(influx_queue, &data, 0);
            // sdcard_log_th(data.temperature, data.humidity);

            /* Optional: update UI safely */
            // lv_async_call(update_ui_cb, &data);
        } else {
            ESP_LOGW("MAIN", "AHT20 read failed");
        }

        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}

void influx_task(void *pv)
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(device_id, sizeof(device_id), "CI-TC-%02X%02X%02X",
                mac[3], mac[4], mac[5]);

    influx_init(device_id);

    while (1) {

        if (xQueueReceive(influx_queue, &data, portMAX_DELAY)) {

            influx_write(data.temperature, data.humidity);
        }
    }
}

static void updateWIFIinfo(void)
{
    // IP
    esp_netif_ip_info_t ip;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif && esp_netif_get_ip_info(netif, &ip) == ESP_OK) {
        snprintf(info.ip, sizeof(info.ip),
                 "IP: " IPSTR, IP2STR(&ip.ip));
    }

    // AP
    wifi_ap_record_t ap;
    if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
        snprintf(info.ap, sizeof(info.ap),
                 "AP: %s", ap.ssid);
    }
    //MAC
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    snprintf(info.mac, sizeof(info.mac),
         "%02X:%02X:%02X:%02X:%02X:%02X",
         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    //Device ID
    snprintf(info.device_id, sizeof(info.device_id), "CI-TC-%02X%02X%02X",
                mac[3], mac[4], mac[5]);

    snprintf(info.temp, sizeof(info.temp),"%.2f", data.temperature);
    snprintf(info.humidity, sizeof(info.humidity), "%.2f", data.humidity);
}

/**
 * @brief this is an exemple of a callback that you can setup in your own app to get notified of wifi manager event.
 */
void cb_connection_ok(void *pvParameter){

    static bool ota_started = false;

	ip_event_got_ip_t* param = (ip_event_got_ip_t*)pvParameter;

	/* transform IP to human readable string */
	char str_ip[16];
	esp_ip4addr_ntoa(&param->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);

	ESP_LOGI(TAG, "I have a connection and my IP is %s!", str_ip);

    updateWIFIinfo();
    lcd_post_wifi_info(&info);
    
    if (!ota_started) {

        char fetched_version[16];

        fetch_version(fetched_version, sizeof(fetched_version));
        
        fetched_version[strcspn(fetched_version, "\r\n")] = 0;

        const esp_app_desc_t *app_desc = esp_app_get_description();
        const char *current_version = app_desc->version;

        ESP_LOGI("OTA", "Current version: %s", current_version);
        ESP_LOGI("OTA", "Server version: %s", fetched_version);

        if (strcmp(fetched_version, current_version) != 0) {
            ESP_LOGI("OTA", "New version available: %s", fetched_version);
            simple_ota_start();
            ota_started = true;
        } 
        else 
        {
            ESP_LOGI("OTA", "Device already up-to-date");
        }
    }
}

void app_main(void)
{

    /* Init I2C + sensor first */
    i2c_init();
    aht20_init(I2C_MASTER_NUM);

    influx_queue = xQueueCreate(10, sizeof(aht20_data_t));

    /* Init SD card */
    // if (sdcard_init() != ESP_OK) {
    //     ESP_LOGE("MAIN", "SD card init failed");
    //     return;
    // }
    
    initialize_display();

	/* start the wifi manager */
	wifi_manager_start();
    /* register a callback as an example to how you can integrate your code with the wifi manager */
	wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &cb_connection_ok);

    /* your code should go here. Here we simply create a task on core 2 that monitors free heap memory */
	xTaskCreatePinnedToCore(&monitoring_task, "monitoring_task", 2048, NULL, 4, NULL, 1);
    
    xTaskCreatePinnedToCore(sensor_task, "sensor_task",4096, NULL, 2, NULL, 0);

    xTaskCreatePinnedToCore(influx_task, "influx_task",4096, NULL, 3, NULL, 1);
}

