// Em: temp_hum_monitoring.c

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/cyw43_arch.h"
#include "temperature_sensor.h"
#include "http_client.h" // (ou web_server.h)
#include "wifi_connection.h"

// Use 5 segundos para teste (5000 ms)
#define DELAY_ENTRE_REQUISICOES_MS 5000 

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

    char *id = "placa_jp"; // (Use o ID correto)
    float temp, hum;
    
    // --- ESTE É O NOVO LOOP "INTELIGENTE" ---
    while (true)
    {
        // 1. A rede está ocupada?
        if (is_http_request_in_progress())
        {
            // Sim. Não faz NADA, só espera 100ms
            // e checa de novo.
            sleep_ms(100);
        }
        else 
        {
            // 2. A rede está LIVRE!
            // Agora sim lemos o sensor e enviamos.
            printf("Rede livre, lendo sensor...\n");
            if (aht10_read(&temp, &hum))
            {
                // Inicia a *próxima* requisição.
                // (Isso vai setar a flag "g_request_in_progress" para true)
                create_request(id, temp, hum);
            }
            else
            {
                printf("Falha ao ler sensor.\n");
            }

            // 3. Espera o delay para a próxima leitura.
            // O loop só vai voltar para o passo 1
            // daqui a 5 segundos.
            sleep_ms(DELAY_ENTRE_REQUISICOES_MS);
        }
    }
    // --- FIM DO NOVO LOOP ---
    
    cyw43_arch_deinit();
    return 0;
}