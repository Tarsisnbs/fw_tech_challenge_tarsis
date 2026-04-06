#include "sensors.h"
#include "storage.h"
#include "FreeRTOS.h"
#include "task.h"

// Note que aqui usamos as funções de lock do storage para proteger os dados
void sensors_touch_alive(uint8_t sensor_id) {
    if (storage_lock(pdMS_TO_TICKS(10))) {
        sensor_data_t* s = storage_get_sensor(sensor_id);
        if (s) {
            s->registered = true;
            s->last_rx_tick = xTaskGetTickCount();
        }
        storage_unlock();
    }
}

bool sensors_check_for_timeout(uint32_t timeout_ms) {
    bool fault = false;
    TickType_t now = xTaskGetTickCount();
    TickType_t limit = pdMS_TO_TICKS(timeout_ms);

    if (storage_lock(pdMS_TO_TICKS(20))) {
        for (int i = 0; i < MAX_SENSORS; i++) {
            sensor_data_t* s = storage_get_sensor(i);
            if (s && s->registered) {
                if ((now - s->last_rx_tick) > limit) {
                    fault = true;
                    break;
                }
            }
        }
        storage_unlock();
    }
    return fault;
}