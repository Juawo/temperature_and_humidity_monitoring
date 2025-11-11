#ifndef WIFI_CONNECTION_H
#define WIFI_CONNECTION_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#define WIFI_SSID "Vertical20"
#define WIFI_PASSWORD "Soft#2025"// Caso a rede não possua senha, coloque NULL

void setup_wifi();

#endif