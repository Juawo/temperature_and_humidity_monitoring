// Em: src/http_client.c (ou web_server.c)

#include "http_client.h" // (ou web_server.h)
#include <stdbool.h>

static volatile bool g_request_in_progress = false;

typedef struct HTTP_REQUEST_STATE_T
{
    struct tcp_pcb *pcb;
    char json_payload[256];
    ip_addr_t server_ip;
} HTTP_REQUEST_STATE_T;

/**
 * @brief Libera a memória e fecha a conexão. (Esta é a função CRÍTICA)
 */
static err_t http_close_connection(HTTP_REQUEST_STATE_T *state)
{
    err_t err = ERR_OK;
    if (state->pcb != NULL)
    {
        tcp_sent(state->pcb, NULL);
        tcp_err(state->pcb, NULL);
        tcp_recv(state->pcb, NULL);
        err = tcp_close(state->pcb);
        if (err != ERR_OK)
        {
            printf("Erro ao fechar PCB: %d, abortando.\n", err);
            tcp_abort(state->pcb);
            err = ERR_ABRT;
        }
        state->pcb = NULL;
    }

    if (state)
    {
        free(state);
    }

    // A LINHA MAIS IMPORTANTE:
    // Diz ao 'main' loop que a rede está livre.
    g_request_in_progress = false;
    printf("Conexão fechada. Rede está livre.\n");
    return err;
}

/**
 * @brief Callback: Dados enviados com sucesso.
 */
static err_t http_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
    HTTP_REQUEST_STATE_T *state = (HTTP_REQUEST_STATE_T *)arg;
    printf("Dados enviados com sucesso (Bytes: %d)!\n", len);
    return http_close_connection(state); // Fecha e libera a flag
}

/**
 * @brief Callback: Conexão TCP estabelecida.
 */
static err_t http_connect_callback(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    if (err != ERR_OK)
    {
        printf("Erro ao conectar ao servidor: %d\n", err);
        return http_close_connection((HTTP_REQUEST_STATE_T *)arg); // CORREÇÃO: Chama o close no erro
    }

    HTTP_REQUEST_STATE_T *state = (HTTP_REQUEST_STATE_T *)arg;
    printf("Conectado ao servidor! Enviando POST...\n");

    // Monta a requisição HTTP
    char request_buffer[512];
    snprintf(request_buffer, sizeof(request_buffer),
             "POST %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             SERVER_PATH, SERVER_HOST,
             strlen(state->json_payload), state->json_payload);

    printf("Requisição HTTP:\n%s\n", request_buffer);
    tcp_sent(tpcb, http_sent_callback);

    cyw43_arch_lwip_begin();
    err_t write_err = tcp_write(tpcb, request_buffer, strlen(request_buffer), TCP_WRITE_FLAG_COPY);
    cyw43_arch_lwip_end();

    if (write_err != ERR_OK)
    {
        printf("Erro ao enviar dados (tcp_write): %d\n", write_err);
        return http_close_connection(state); // CORREÇÃO: Chama o close no erro
    }

    return ERR_OK;
}

/**
 * @brief Callback: DNS encontrou o IP.
 */
static void http_dns_found_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
    HTTP_REQUEST_STATE_T *state = (HTTP_REQUEST_STATE_T *)callback_arg;

    if (ipaddr == NULL)
    {
        printf("Falha no DNS: Host '%s' nao encontrado.\n", name);
        // CORREÇÃO: Chama o close no erro
        http_close_connection(state);
        return;
    }

    state->server_ip = *ipaddr;
    printf("DNS OK: Host '%s' tem o IP: %s\n", name, ip4addr_ntoa(ipaddr));

    cyw43_arch_lwip_begin();
    state->pcb = tcp_new();
    if (state->pcb == NULL)
    {
        printf("Erro ao criar pcb\n");
        cyw43_arch_lwip_end();
        // CORREÇÃO: Chama o close no erro
        http_close_connection(state);
        return;
    }

    tcp_arg(state->pcb, state);
    err_t err = tcp_connect(state->pcb, &state->server_ip, SERVER_PORT, http_connect_callback);
    cyw43_arch_lwip_end();

    if (err != ERR_OK)
    {
        printf("Erro ao iniciar conexao TCP: %d\n", err);
        // CORREÇÃO: Chama o close no erro
        http_close_connection(state);
    }
}

/**
 * @brief Função principal que o main.c vai chamar.
 */
void create_request(char *id, float temperature, float humidity)
{
    if (g_request_in_progress)
    {
        // Esta mensagem não deveria mais aparecer se o 'main' estiver correto
        printf("HTTP em progresso, pulando esta medição.\n");
        return;
    }

    g_request_in_progress = true; // Seta a flag

    HTTP_REQUEST_STATE_T *state = (HTTP_REQUEST_STATE_T *)calloc(1, sizeof(HTTP_REQUEST_STATE_T));
    if (state == NULL)
    {
        printf("Falha ao alocar memoria para o state\n");
        g_request_in_progress = false; // CORREÇÃO: Libera a flag se 'calloc' falhar
        return;
    }

    snprintf(state->json_payload, sizeof(state->json_payload),
             "{ \"temperatura\" : %.2f, "
             "\"umidade\" : %.2f, "
             "\"codigoPlaca\" : \"%s\" }",
             temperature, humidity, id);

    printf("JSON gerado: %s\n", state->json_payload);

    cyw43_arch_lwip_begin();
    err_t err = dns_gethostbyname(SERVER_HOST, &state->server_ip, http_dns_found_callback, state);
    cyw43_arch_lwip_end();

    if (err == ERR_OK)
    {
        // O IP já estava em cache, o callback foi chamado imediatamente
        // A flag será limpa dentro do callback
    }
    else if (err != ERR_INPROGRESS)
    {
        printf("Erro ao iniciar DNS: %d\n", err);
        // CORREÇÃO: Chama o close no erro
        http_close_connection(state);
    }
    // Se err == ERR_INPROGRESS, esperamos a rede...
}

/**
 * @brief Função "getter" de status (para o main.c).
 */
bool is_http_request_in_progress(void)
{
    return g_request_in_progress;
}