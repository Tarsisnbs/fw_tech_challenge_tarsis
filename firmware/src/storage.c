#include "storage.h"
#include <string.h>

static sensor_data_t sensor_vault[MAX_SENSORS];
static SemaphoreHandle_t xVaultMutex = NULL;
static StaticSemaphore_t xVaultMutexBuffer;

void storage_init(void) {
    // Initializes the Mutex statically.
    xVaultMutex = xSemaphoreCreateMutexStatic(&xVaultMutexBuffer);
    
    // Clear the vault's memory.
    memset(sensor_vault, 0, sizeof(sensor_vault));
}

bool storage_save_packet(uint8_t sensor_id, uint8_t *payload, uint8_t len) {
    if (sensor_id >= MAX_SENSORS || payload == NULL || len > PAYLOAD_SIZE) {
        return false;
    }

    // Try to get the Mutex (short timeout so as not to crash the Parser).
    if (xSemaphoreTake(xVaultMutex, portMAX_DELAY) == pdTRUE) {
        sensor_data_t *s = &sensor_vault[sensor_id];
        
        // Calculates the Ring Buffer Index
        uint8_t slot = s->head;

        // salva payload
        memcpy(s->slots[slot], payload, len);
        s->len[slot] = len;

        // atualiza último payload
        memcpy(s->last_payload, payload, len);
        s->last_len = len;

        // avança ring buffer
        s->head = (s->head + 1) % RING_BUFFER_SIZE;

        s->sample_count++;
        s->last_rx_tick = xTaskGetTickCount();
        s->registered = true;

        xSemaphoreGive(xVaultMutex);
        return true;
    }
    return false;
}

sensor_data_t* storage_get_sensor(uint8_t sensor_id) {
    if (sensor_id >= MAX_SENSORS) return NULL;
    return &sensor_vault[sensor_id];
}

bool storage_lock(TickType_t xTicksToWait) {
    return (xSemaphoreTake(xVaultMutex, xTicksToWait) == pdTRUE);
}

void storage_unlock(void) {
    xSemaphoreGive(xVaultMutex);
}