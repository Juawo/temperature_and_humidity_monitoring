#include "temperature_sensor.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <stdio.h>

#define AHT10_CMD_INIT     0xE1
#define AHT10_CMD_TRIGGER  0xAC
#define AHT10_CMD_SOFT_RST 0xBA

static bool aht10_wait_ready() {
    uint8_t status;
    const absolute_time_t timeout = make_timeout_time_ms(100);
    do {
        i2c_read_blocking(i2c0, AHT10_ADDR, &status, 1, false);
        if (!(status & 0x80)) return true;  // Bit 7 = 0 -> pronto
    } while (!time_reached(timeout));
    return false;
}

bool aht10_init() {
    // Envia comando de inicialização (pode ser omitido em alguns sensores)
    uint8_t cmd[] = { AHT10_CMD_INIT, 0x08, 0x00 };
    int ret = i2c_write_blocking(i2c0, AHT10_ADDR, cmd, sizeof(cmd), false);
    sleep_ms(20);  // espera inicial
    return ret >= 0;
}

bool aht10_read(float *temperature, float *humidity) {
    // Comando de medição: 0xAC, 0x33, 0x00
    uint8_t cmd[] = { AHT10_CMD_TRIGGER, 0x33, 0x00 };
    if (i2c_write_blocking(i2c0, AHT10_ADDR, cmd, sizeof(cmd), false) < 0)
        return false;

    sleep_ms(80);  // aguarda conversão

    // Lê 6 bytes: status + 5 dados
    uint8_t data[6];
    if (i2c_read_blocking(i2c0, AHT10_ADDR, data, 6, false) < 0)
        return false;

    // Verifica se pronto
    if (data[0] & 0x80)
        return false;

    // Extrai umidade (20 bits)
    uint32_t hum_raw = ((uint32_t)(data[1]) << 12) |
                       ((uint32_t)(data[2]) << 4) |
                       ((uint32_t)(data[3]) >> 4);

    // Extrai temperatura (20 bits)
    uint32_t temp_raw = ((uint32_t)(data[3] & 0x0F) << 16) |
                        ((uint32_t)(data[4]) << 8) |
                        ((uint32_t)(data[5]));

    *humidity = ((float)hum_raw * 100.0f) / 1048576.0f;
    *temperature = ((float)temp_raw * 200.0f) / 1048576.0f - 50.0f;

    return true;
}
