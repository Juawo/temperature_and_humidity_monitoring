#ifndef MAX30100_H
#define MAX30100_H

#include "hardware/i2c.h"

// Função para inicializar o MAX30100
void init_max30100(i2c_inst_t *i2c_port);

// Função para ler os dados do MAX30100 (BPM e SpO2)
void read_max30100(i2c_inst_t *i2c_port, float *bpm, float *spo2);

#endif
