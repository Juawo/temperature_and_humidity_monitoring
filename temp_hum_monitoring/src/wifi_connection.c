#include "wifi_connection.h"

void setup_wifi()
{
    if(cyw43_arch_init() != 0)
    {
        printf("Erro ao inicializar Wi-Fi\n");
        return;
    }

    cyw43_arch_enable_sta_mode();

    while(cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000))
    {
        printf("Erro ao conectar ao Wi-Fi, tentando novamente\n");
        sleep_ms(3000);
    }

    printf("Conectado ao Wi-Fi!\n");
}