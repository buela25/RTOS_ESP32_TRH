#include "sdcard.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "driver/spi_common.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

#define TAG "SDCARD"
#define MOUNT_POINT "/sdcard"
#define CSV_FILE "/sdcard/TRH_log.csv"

/* SPI pins */
#define PIN_NUM_MISO  19
#define PIN_NUM_MOSI  23
#define PIN_NUM_CLK   18
#define PIN_NUM_CS     5

static sdmmc_card_t *card;

/* -------------------------------------------------- */
/* SD card init                                       */
/* -------------------------------------------------- */
esp_err_t sdcard_init(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Initializing SD card (SPI)");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed");
        return ret;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 3,
    };

    ret = esp_vfs_fat_sdspi_mount(
        MOUNT_POINT,
        &host,
        &slot_config,
        &mount_config,
        &card
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD mount failed");
        spi_bus_free(host.slot);
        return ret;
    }

    sdmmc_card_print_info(stdout, card);
    ESP_LOGI(TAG, "SD card mounted at %s", MOUNT_POINT);

    return ESP_OK;
}

/* -------------------------------------------------- */
/* Create CSV header if needed                         */
/* -------------------------------------------------- */
static void csv_init(void)
{
    struct stat st;
    if (stat(CSV_FILE, &st) == 0) {
        return; // CSV already exists
    }

    FILE *f = fopen(CSV_FILE, "w");
    if (!f) {
        ESP_LOGE(TAG, "CSV create failed");
        return;
    }

    fprintf(f, "temperature,humidity\n");
    fclose(f);

    ESP_LOGI(TAG, "CSV header created");
}

/* -------------------------------------------------- */
/* Append temperature & humidity                       */
/* -------------------------------------------------- */
esp_err_t sdcard_log_th(float temperature, float humidity)
{
    csv_init();

    FILE *f = fopen(CSV_FILE, "a");
    if (!f) {
        ESP_LOGE(TAG, "CSV open failed");
        return ESP_FAIL;
    }

    fprintf(f, "%.2f,%.2f\n", temperature, humidity);
    fclose(f);

    ESP_LOGI(TAG, "Logged T=%.2f H=%.2f", temperature, humidity);
    return ESP_OK;
}

/* -------------------------------------------------- */
/* Deinit                                             */
/* -------------------------------------------------- */
void sdcard_deinit(void)
{
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    spi_bus_free(SPI2_HOST);
    ESP_LOGI(TAG, "SD card unmounted");
}
