#include "aht20.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define AHT20_CMD_INIT     0xBE
#define AHT20_CMD_TRIGGER  0xAC
#define AHT20_CMD_SOFT_RST 0xBA

static esp_err_t aht20_write(i2c_port_t i2c_num, uint8_t *data, size_t len)
{
    return i2c_master_write_to_device(
        i2c_num,
        AHT20_I2C_ADDR,
        data,
        len,
        pdMS_TO_TICKS(1000)
    );
}

static esp_err_t aht20_read_raw(i2c_port_t i2c_num, uint8_t *data, size_t len)
{
    return i2c_master_read_from_device(
        i2c_num,
        AHT20_I2C_ADDR,
        data,
        len,
        pdMS_TO_TICKS(1000)
    );
}

esp_err_t aht20_init(i2c_port_t i2c_num)
{
    uint8_t cmd[] = { AHT20_CMD_INIT, 0x08, 0x00 };
    vTaskDelay(pdMS_TO_TICKS(40));
    return aht20_write(i2c_num, cmd, sizeof(cmd));
}

esp_err_t aht20_read(i2c_port_t i2c_num, aht20_data_t *data)
{
    uint8_t cmd[] = { AHT20_CMD_TRIGGER, 0x33, 0x00 };
    uint8_t raw[6];

    ESP_ERROR_CHECK(aht20_write(i2c_num, cmd, sizeof(cmd)));
    vTaskDelay(pdMS_TO_TICKS(80));

    ESP_ERROR_CHECK(aht20_read_raw(i2c_num, raw, sizeof(raw)));

    uint32_t hum_raw =
        ((uint32_t)raw[1] << 12) |
        ((uint32_t)raw[2] << 4) |
        (raw[3] >> 4);

    uint32_t temp_raw =
        ((uint32_t)(raw[3] & 0x0F) << 16) |
        ((uint32_t)raw[4] << 8) |
        raw[5];

    data->humidity = ((float)hum_raw / 1048576.0f) * 100.0f;
    data->temperature = ((float)temp_raw / 1048576.0f) * 200.0f - 50.0f;

    return ESP_OK;
}
