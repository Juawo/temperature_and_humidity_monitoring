#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lwip/netif.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "lwip/dns.h" // <-- PRECISAMOS DISSO
#include <stdbool.h>


// ATUALIZE COM SEUS DADOS DO NGROK
#define SERVER_HOST "madalyn-thoroughgoing-continuedly.ngrok-free.dev"
#define SERVER_PORT 80
#define SERVER_PATH "/dados" // Mude para "/dados" ou seu endpoint real

/**
 * @brief Prepara o JSON e inicia a requisição HTTP POST assíncrona.
 * * Esta função não envia os dados imediatamente. Ela formata o JSON
 * e inicia o processo de "Resolução de DNS" (descobrir o IP do host).
 * O envio real acontece em callbacks.
 * * @param id 
 * @param temperature 
 * @param humidity 
 */
void create_request(char *id, float temperature, float humidity);
bool is_http_request_in_progress(void);

#endif