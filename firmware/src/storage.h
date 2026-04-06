#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "semphr.h"

#define MAX_SENSORS       4
#define RING_BUFFER_SIZE  8
#define PAYLOAD_SIZE      16

typedef struct {
    uint8_t    slots[RING_BUFFER_SIZE][PAYLOAD_SIZE];
    uint8_t    head; //control the buffer rotation (0 to 7
    uint8_t len[RING_BUFFER_SIZE];
    uint8_t    last_payload[PAYLOAD_SIZE];
    uint8_t last_len; 
    
    uint32_t   sample_count;
    TickType_t last_rx_tick;
    bool       registered;
} sensor_data_t;

/* Initialize the Mutex and clear the Vault.*/
void storage_init(void);

/* Save a new package coming from the Parser.*/
bool storage_save_packet(uint8_t sensor_id, uint8_t *payload, uint8_t len);

/* 
Returns a pointer to the sensor data (for the shell to read) 
IMPORTANT: Must be used between storage_lock() and storage_unlock 
   */
sensor_data_t* storage_get_sensor(uint8_t sensor_id);

/* Access control functions for the Shell */
bool storage_lock(TickType_t xTicksToWait);
void storage_unlock(void);

#endif