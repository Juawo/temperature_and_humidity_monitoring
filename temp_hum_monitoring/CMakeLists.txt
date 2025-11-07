# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Configuração do VS Code (Deixe como está) ---
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# --- Fim da Configuração ---

set(PICO_BOARD pico_w CACHE STRING "Board type")

# Puxar o SDK
include(pico_sdk_import.cmake)

project(temp_hum_monitoring C CXX ASM)

# Inicializar o SDK
pico_sdk_init()

# Adicionar o executável com todos os seus arquivos .c
add_executable(temp_hum_monitoring 
    temp_hum_monitoring.c 
    src/temperature_sensor.c
    src/http_client.c        # (Use web_server.c, o último que te mandei)
    src/wifi_connection.c
)

# Configurar STDI/O (Saída de printfs)
pico_enable_stdio_uart(temp_hum_monitoring 0)
pico_enable_stdio_usb(temp_hum_monitoring 1)

# Adicionar os includes (pastas .h)
target_include_directories(temp_hum_monitoring PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        include
)

# --- AQUI ESTÁ A CORREÇÃO ---
# Linkar todas as bibliotecas necessárias em UM só bloco
target_link_libraries(temp_hum_monitoring PRIVATE
    pico_stdlib
    hardware_i2c
    hardware_gpio

    # Esta é a biblioteca "mágica" que inclui
    # Wi-Fi, lwIP, TCP e DNS, tudo que precisamos.
    pico_cyw43_arch_lwip_threadsafe_background
)

# Gerar os arquivos .uf2, .elf
pico_add_extra_outputs(temp_hum_monitoring)