#ifndef INFLUX_H
#define INFLUX_H

void influx_init(const char* device_id);
void influx_write(float temperature, float humidity);
void influx_deinit(void);

#endif