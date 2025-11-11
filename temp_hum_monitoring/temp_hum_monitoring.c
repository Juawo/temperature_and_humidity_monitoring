// Em: temp_hum_monitoring.c

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/cyw43_arch.h"
#include "temperature_sensor.h"
#include "http_client.h" // (ou web_server.h)
#include "wifi_connection.h"

// Defina o intervalo desejado
#define DELAY_ENTRE_REQUISICOES_MS 30000 // 30 segundos

int main()
{
    stdio_init_all();
    setup_wifi();
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    char *id = "placa_kd";
    float temp, hum;

    while (true)
    {
        // 1. A rede está ocupada?
        if (is_http_request_in_progress())
        {
            sleep_ms(100);
        }
        else 
        {
            // 2. A rede está LIVRE!
            printf("Rede livre, lendo sensor...\n");
            if (aht10_read(&temp, &hum))
            {
                create_request(id, temp, hum);
            }
            else
            {
                printf("Falha ao ler sensor.\n");
            }

            // 3. Espera o delay para a próxima tentativa
            sleep_ms(DELAY_ENTRE_REQUISICOES_MS);
        }
    }
}