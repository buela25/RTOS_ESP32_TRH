#ifndef AHT20_H
#define AHT20_H

#include "driver/i2c.h"
#include "esp_err.h"

#define AHT20_I2C_ADDR  0x38

typedef struct {
    float temperature;
    float humidity;
} aht20_data_t;

esp_err_t aht20_init(i2c_port_t i2c_num);
esp_err_t aht20_read(i2c_port_t i2c_num, aht20_data_t *data);

#endif
