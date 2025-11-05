cmake_minimum_required(VERSION 3.13)

# Encontra o SDK do Pico
include(pico_sdk_import.cmake)

project(https-request-test C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Inicializa o SDK
pico_sdk_init()

add_executable(https-request-test
    https-request-test.c
)

# Configuração do STDOUT/STDIN via USB
pico_enable_stdio_uart(https-request-test 0)
pico_enable_stdio_usb(https-request-test 1)

# --- Esta é a parte mais importante ---
# Você precisa linkar todas as dependências:
# 1. stdlib (padrão)
# 2. cyw43_arch... (para Wi-Fi + LwIP)
# 3. pico_lwip (a pilha TCP/IP)
# 4. pico_dns (o cliente DNS)
# 5. pico_mbedtls (a biblioteca SSL/TLS)
target_link_libraries(https-request-test
    pico_stdlib
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip
    pico_dns
    pico_mbedtls
)

# Adiciona o .uf2 e outros formatos de saída
pico_add_extra_outputs(https-request-test)