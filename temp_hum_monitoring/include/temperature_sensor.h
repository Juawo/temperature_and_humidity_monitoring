#ifndef TEMP_HUMI_SENSOR_H
#define TEMP_HUMI_SENSOR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define I2C_PORT i2c0
#define I2C_SDA 0
#define I2C_SCL 1
#define AHT10_ADDR 0x38
#define AHT10_CMD_INIT     0xE1
#define AHT10_CMD_TRIGGER  0xAC
#define AHT10_CMD_SOFT_RST 0xBA

bool aht10_init();
bool aht10_read(float *temperature, float *humidity);

#endif