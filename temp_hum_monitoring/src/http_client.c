#include "http_client.h"

// Esta estrutura guarda os dados da requisição
// enquanto ela acontece em background.
typedef struct HTTP_REQUEST_STATE_T
{
    struct tcp_pcb *pcb;    // O "controle" da conexão TCP
    char json_payload[256]; // O JSON que queremos enviar
    ip_addr_t server_ip;    // O IP do servidor (que vamos descobrir)
} HTTP_REQUEST_STATE_T;

/**
 * @brief Libera a memória e fecha a conexão.
 * Chamado no final ou em caso de erro.
 */
static err_t http_close_connection(HTTP_REQUEST_STATE_T *state)
{
    err_t err = ERR_OK;
    if (state->pcb != NULL)
    {
        tcp_sent(state->pcb, NULL); // Remove callbacks
        tcp_err(state->pcb, NULL);
        tcp_recv(state->pcb, NULL);
        err = tcp_close(state->pcb);
        if (err != ERR_OK)
        {
            printf("Erro ao fechar PCB: %d\n", err);
            tcp_abort(state->pcb); // Força o fechamento
            err = ERR_ABRT;
        }
        state->pcb = NULL;
    }

    // Libera a memória da estrutura
    if (state)
    {
        free(state);
    }
    return err;
}

/**
 * @brief Callback: Chamado quando os dados são enviados com sucesso.
 */
static err_t http_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
    HTTP_REQUEST_STATE_T *state = (HTTP_REQUEST_STATE_T *)arg;
    printf("Dados enviados com sucesso (Bytes: %d)!\n", len);

    // Dados enviados, podemos fechar a conexão
    return http_close_connection(state);
}

/**
 * @brief Callback: Chamado quando a conexão TCP é estabelecida.
 * É AQUI que enviamos nosso POST.
 */
static err_t http_connect_callback(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    if (err != ERR_OK)
    {
        printf("Erro ao conectar ao servidor: %d\n", err);
        return http_close_connection((HTTP_REQUEST_STATE_T *)arg);
    }

    HTTP_REQUEST_STATE_T *state = (HTTP_REQUEST_STATE_T *)arg;
    printf("Conectado ao servidor! Enviando POST...\n");

    // Monta a requisição HTTP (igual ao seu código, mas com HostName)
    char request_buffer[512];
    snprintf(request_buffer, sizeof(request_buffer),
             "POST %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n" // Pedimos para fechar após a resposta
             "\r\n"
             "%s",
             SERVER_PATH,
             SERVER_HOST, // <-- MUITO IMPORTANTE para o ngrok
             strlen(state->json_payload),
             state->json_payload);

    printf("Requisição HTTP:\n%s\n", request_buffer);

    // Registra o callback de "dados enviados"
    tcp_sent(tpcb, http_sent_callback);

    // Escreve os dados na conexão TCP
    cyw43_arch_lwip_begin();
    err_t write_err = tcp_write(tpcb, request_buffer, strlen(request_buffer), TCP_WRITE_FLAG_COPY);
    cyw43_arch_lwip_end();

    if (write_err != ERR_OK)
    {
        printf("Erro ao enviar dados (tcp_write): %d\n", write_err);
        return http_close_connection(state);
    }

    return ERR_OK;
}

/**
 * @brief Callback: Chamado quando o DNS descobre o IP do HostName.
 */
static void http_dns_found_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
    if (ipaddr == NULL)
    {
        printf("Falha no DNS: Host '%s' nao encontrado.\n", name);
        free(callback_arg); // Libera o 'state' em caso de falha
        return;
    }

    HTTP_REQUEST_STATE_T *state = (HTTP_REQUEST_STATE_T *)callback_arg;
    state->server_ip = *ipaddr;
    printf("DNS OK: Host '%s' tem o IP: %s\n", name, ip4addr_ntoa(ipaddr));

    // 2. Agora que temos o IP, vamos criar o PCB e conectar
    cyw43_arch_lwip_begin();
    state->pcb = tcp_new();
    if (state->pcb == NULL)
    {
        printf("Erro ao criar pcb\n");
        cyw43_arch_lwip_end();
        free(state);
        return;
    }

    // Passa o 'state' para o próximo callback
    tcp_arg(state->pcb, state);

    // 3. Inicia a conexão TCP
    err_t err = tcp_connect(state->pcb, &state->server_ip, SERVER_PORT, http_connect_callback);
    cyw43_arch_lwip_end();

    if (err != ERR_OK)
    {
        printf("Erro ao iniciar conexao TCP: %d\n", err);
        http_close_connection(state); // Limpa em caso de erro
    }
}

/**
 * @brief Função principal que seu main.c vai chamar.
 */
void create_request(char *id, float temperature, float humidity)
{
    // 1. Aloca memória para o "estado" da requisição
    // (Será liberado no 'http_close_connection')
    HTTP_REQUEST_STATE_T *state = (HTTP_REQUEST_STATE_T *)calloc(1, sizeof(HTTP_REQUEST_STATE_T));
    if (state == NULL)
    {
        printf("Falha ao alocar memoria para o state\n");
        return;
    }

    // Formata o JSON e guarda no "state"
    snprintf(state->json_payload, sizeof(state->json_payload),
             "{ \"temperatura\" : %.2f, "
             "\"umidade\" : %.2f, "
             "\"codigoPlaca\" : \"%s\" }",
             temperature, humidity, id);

    printf("JSON gerado: %s\n", state->json_payload);

    // 1. Inicia o processo de DNS
    // (O resto acontece nos callbacks)
    cyw43_arch_lwip_begin();
    err_t err = dns_gethostbyname(SERVER_HOST,
                                  &state->server_ip,
                                  http_dns_found_callback,
                                  state); // Passa o 'state' como argumento
    cyw43_arch_lwip_end();

    if (err == ERR_OK)
    {
        // O IP já estava em cache, o callback foi chamado imediatamente
        // (não precisamos fazer nada, o callback já cuidou)
    }
    else if (err != ERR_INPROGRESS)
    {
        printf("Erro ao iniciar DNS: %d\n", err);
        free(state); // Libera o state em caso de falha
    }
    // Se err == ERR_INPROGRESS, estamos aguardando a rede responder...
}