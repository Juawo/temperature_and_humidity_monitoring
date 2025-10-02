#include "hardware/i2c.h"
#include <stdio.h>

// Endereço I2C para leitura e escrita do MAX30100
#define MAX30100_ADDR_WRITE 0xAE
#define MAX30100_ADDR_READ  0xAF

// Função para inicializar o MAX30100
void init_max30100(i2c_inst_t *i2c_port) {
    uint8_t data[2];
    data[0] = 0x06;  // Comando de configuração
    data[1] = 0x00;  // Valor de configuração (ativando o sensor)
    i2c_write_blocking(i2c_port, MAX30100_ADDR_WRITE, data, 2, false);
}

// Função para ler os dados do MAX30100
void read_max30100(i2c_inst_t *i2c_port, float *bpm, float *spo2) {
    uint8_t data[4];

    // Enviar comando para solicitar dados (registro 0x07)
    i2c_write_blocking(i2c_port, MAX30100_ADDR_WRITE, (uint8_t[]){0x07}, 1, true);

    // Ler os dados (4 bytes)
    i2c_read_blocking(i2c_port, MAX30100_ADDR_READ, data, 4, false);

    uint16_t ir = (data[0] << 8) | data[1];  // LED infravermelho
    uint16_t red = (data[2] << 8) | data[3]; // LED vermelho

    // Calcular BPM e SpO2 (exemplo simples)
    *bpm = (float)(60 + (ir % 40));  // Exemplo de cálculo para BPM
    *spo2 = (float)(95 + (red % 5));  // Exemplo de cálculo para SpO2
}
