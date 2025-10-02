#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/cyw43_arch.h"
#include "temperature_sensor.h"
#include "web_server.h"
#include "wifi_connection.h"
#include "max30100.h"  // Incluir o cabeçalho do MAX30100

int main() {
    stdio_init_all();
    setup_wifi();

    // Inicialização do I2C
    i2c_init(I2C_PORT, 400 * 1000); // I2C em 400kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializar o sensor MAX30100
    init_max30100(I2C_PORT);

    // Exemplo de Identificador da placa
    char* id = "placa_kd";
    float temp, hum, bpm, spo2;

    while (true) {
        if (aht10_read(&temp, &hum)) {
            // Ler os dados do MAX30100
            read_max30100(I2C_PORT, &bpm, &spo2);
            
            // Registrar os dados com o ID da placa, temperatura, umidade, BPM e SpO2
            create_request(id, temp, hum, bpm, spo2);
        }

        sleep_ms(2000); // Espera de 2 segundos entre as leituras
    }

    cyw43_arch_deinit();
    return 0;
}
