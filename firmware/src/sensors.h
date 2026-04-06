#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>
#include <stdbool.h>

// Inicializa a estrutura de controle de sensores
void sensors_init(void);

// Notifica a camada 4 que um sensor enviou um pacote válido
void sensors_touch_alive(uint8_t sensor_id);

// Varre todos os sensores registrados e retorna true se algum falhou (timeout)
bool sensors_check_for_timeout(uint32_t timeout_ms);

#endif