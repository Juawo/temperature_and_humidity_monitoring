#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/cyw43_arch.h"
#include "temperature_sensor.h"
#include "http_client.h"
#include "wifi_connection.h"

int main()
{
    stdio_init_all();
    setup_wifi();

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Example to turn on the Pico W LED
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    // Identificador da placa
    char *id = "placa_jp";
    float temp, hum;
    
    
    while (true)
    {
        if (aht10_read(&temp, &hum))
        {
            create_request(id, temp, hum);
        }
        sleep_ms(3000);
    }
    cyw43_arch_deinit();
    return 0;
}
