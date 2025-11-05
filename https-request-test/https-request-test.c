#include <stdio.h>
#include <string.h> // Necessário para snprintf e strlen

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

// Includes do mbedTLS (note os novos)
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"

// --- Suas Configurações ---
#define WIFI_SSID "EmbarcaTech"
#define WIFI_PASS "1embarcatech@Picos"

// ATENÇÃO: Corrigido - sem "https://", sem espaço, sem porta
#define HOST "madalyn-thoroughgoing-continuedly.ngrok-fr" 
// A porta HTTPS é sempre 443 (o ngrok cuida do redirecionamento para 3000)
#define PORT "443" 
#define REQUEST_PATH "/" // Caminho que você quer acessar (ex: "/")
// -------------------------


// Função de limpeza
void cleanup_ssl(mbedtls_net_context *server_fd, mbedtls_entropy_context *entropy,
                 mbedtls_ctr_drbg_context *ctr_drbg, mbedtls_ssl_context *ssl,
                 mbedtls_ssl_config *conf) {
    mbedtls_net_free(server_fd);
    mbedtls_ssl_free(ssl);
    mbedtls_ssl_config_free(conf);
    mbedtls_ctr_drbg_free(ctr_drbg);
    mbedtls_entropy_free(entropy);
}

int main() {
    stdio_init_all();
    printf("Aguardando console USB...\n");
    sleep_ms(4000); // Dá tempo para o console USB conectar

    if (cyw43_arch_init()) {
        printf("Falha ao inicializar Wi-Fi\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();
    printf("Conectando a %s...\n", WIFI_SSID);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("Falha na conexão Wi-Fi\n");
        cyw43_arch_deinit();
        return 1;
    }
    printf("Wi-Fi conectado! IP: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_default)));

    // 1. Inicialização do mbedTLS
    mbedtls_net_context server_fd;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;

    mbedtls_net_init(&server_fd);
    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);

    int ret;
    char err_buf[512]; // Buffer para mensagens de erro do mbedTLS

    // 2. Setup do Gerador de Números Aleatórios (Essencial para SSL)
    printf("Inicializando gerador de números aleatórios (RNG)...\n");
    // (O "pico_client" é uma string de personalização, pode ser qualquer coisa)
    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                    (const unsigned char *)"pico_client",
                                    strlen("pico_client"))) != 0) {
        mbedtls_strerror(ret, err_buf, sizeof(err_buf));
        printf(" ! falha em mbedtls_ctr_drbg_seed: %d (-0x%x) - %s\n", ret, -ret, err_buf);
        // Não precisamos limpar tudo aqui, pois o programa vai encerrar
        cyw43_arch_deinit();
        return 1;
    }
    printf("RNG inicializado.\n");

    // 3. Conexão de Rede (TCP + DNS)
    printf("Conectando ao host %s na porta %s...\n", HOST, PORT);
    if ((ret = mbedtls_net_connect(&server_fd, HOST, PORT, MBEDTLS_NET_PROTO_TCP)) != 0) {
        mbedtls_strerror(ret, err_buf, sizeof(err_buf));
        printf(" ! falha em mbedtls_net_connect: %d (-0x%x) - %s\n", ret, -ret, err_buf);
        cleanup_ssl(&server_fd, &entropy, &ctr_drbg, &ssl, &conf);
        cyw43_arch_deinit();
        return 1;
    }
    printf("Socket TCP conectado!\n");

    // 4. Configuração do SSL/TLS
    printf("Configurando SSL/TLS...\n");
    if ((ret = mbedtls_ssl_config_defaults(&conf,
                                          MBEDTLS_SSL_IS_CLIENT,
                                          MBEDTLS_SSL_TRANSPORT_STREAM,
                                          MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        mbedtls_strerror(ret, err_buf, sizeof(err_buf));
        printf(" ! falha em mbedtls_ssl_config_defaults: %d (-0x%x) - %s\n", ret, -ret, err_buf);
        cleanup_ssl(&server_fd, &entropy, &ctr_drbg, &ssl, &conf);
        cyw43_arch_deinit();
        return 1;
    }

    // AVISO: Isso é inseguro! Pulando verificação do certificado.
    // Idealmente, você baixaria o certificado CA raiz e o validaria.
    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_NONE); 
    
    // Aponta o RNG para a configuração SSL
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_func, &ctr_drbg);
    
    // Aplica a configuração ao contexto SSL
    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0) {
        mbedtls_strerror(ret, err_buf, sizeof(err_buf));
        printf(" ! falha em mbedtls_ssl_setup: %d (-0x%x) - %s\n", ret, -ret, err_buf);
        cleanup_ssl(&server_fd, &entropy, &ctr_drbg, &ssl, &conf);
        cyw43_arch_deinit();
        return 1;
    }

    // CRÍTICO: Define o hostname para o SNI (Server Name Indication)
    // Servidores como o ngrok usam isso para saber qual site você quer.
    if ((ret = mbedtls_ssl_set_hostname(&ssl, HOST)) != 0) {
        mbedtls_strerror(ret, err_buf, sizeof(err_buf));
        printf(" ! falha em mbedtls_ssl_set_hostname: %d (-0x%x) - %s\n", ret, -ret, err_buf);
        cleanup_ssl(&server_fd, &entropy, &ctr_drbg, &ssl, &conf);
        cyw43_arch_deinit();
        return 1;
    }

    // Define o "BIO" (funções de entrada/saída) para usar o socket de rede
    mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

    // 5. Handshake SSL
    printf("Iniciando Handshake SSL...\n");
    while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            mbedtls_strerror(ret, err_buf, sizeof(err_buf));
            printf(" ! falha em mbedtls_ssl_handshake: %d (-0x%x) - %s\n", ret, -ret, err_buf);
            cleanup_ssl(&server_fd, &entropy, &ctr_drbg, &ssl, &conf);
            cyw43_arch_deinit();
            return 1;
        }
        // Se mbedTLS pedir para ler/escrever, apenas continuamos o loop
    }
    printf("Conexão HTTPS estabelecida!\n");

    // 6. Envia requisição GET
    printf("Enviando requisição HTTP...\n");
    char req_buf[512];
    // CRÍTICO: O 'Host' precisa ser o mesmo do #define HOST
    int req_len = snprintf(req_buf, sizeof(req_buf),
                           "GET %s HTTP/1.1\r\n"
                           "Host: %s\r\n"
                           "Connection: close\r\n"
                           "User-Agent: PicoW-C-Client\r\n"
                           "\r\n",
                           REQUEST_PATH, HOST);

    printf("--- Início da Requisição ---\n%s--- Fim da Requisição ---\n", req_buf);

    while ((ret = mbedtls_ssl_write(&ssl, (const unsigned char *)req_buf, req_len)) <= 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            mbedtls_strerror(ret, err_buf, sizeof(err_buf));
            printf(" ! falha em mbedtls_ssl_write: %d (-0x%x) - %s\n", ret, -ret, err_buf);
            cleanup_ssl(&server_fd, &entropy, &ctr_drbg, &ssl, &conf);
            cyw43_arch_deinit();
            return 1;
        }
    }
    printf("Requisição enviada (%d bytes).\n", ret);

    // 7. Lê resposta
    printf("Lendo resposta do servidor...\n");
    printf("--- Início da Resposta ---\n");
    unsigned char buf[1024];
    int len;
    do {
        memset(buf, 0, sizeof(buf));
        len = mbedtls_ssl_read(&ssl, buf, sizeof(buf) - 1);

        if (len == MBEDTLS_ERR_SSL_WANT_READ || len == MBEDTLS_ERR_SSL_WANT_WRITE) {
            continue; // Tenta ler novamente
        }
        if (len == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
            printf("(Servidor fechou a conexão normalmente)\n");
            break; // Fim da leitura
        }
        if (len < 0) {
            mbedtls_strerror(len, err_buf, sizeof(err_buf));
            printf(" ! falha em mbedtls_ssl_read: %d (-0x%x) - %s\n", len, -len, err_buf);
            break;
        }
        if (len == 0) {
            printf("(Conexão fechada - EOF)\n");
            break; // Fim da leitura
        }

        // Imprime os dados recebidos
        printf("%s", (char *)buf);

    } while (len > 0);
    printf("\n--- Fim da Resposta ---\n");


    // 8. Encerramento
    printf("Encerrando conexão\n");
    mbedtls_ssl_close_notify(&ssl);
    cleanup_ssl(&server_fd, &entropy, &ctr_drbg, &ssl, &conf);
    cyw43_arch_deinit();
    printf("Tudo limpo. Fim.\n");

    return 0;
}