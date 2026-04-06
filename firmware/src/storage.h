#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "semphr.h"

/**
 * @file    storage.h
 * @brief   Thread-safe Data Storage and Persistence Module.
 * @details Manages the internal registry for sensor data. Provides 
 * synchronization primitives (Mutex) to ensure atomic access between 
 * high-priority parsing and low-priority shell reporting.
 */

/* --- Constants & Types --- */
#define MAX_SENSORS       4  /* Scaled to ensure 6KB RAM compliance */
#define RING_BUFFER_SIZE  8  /* Minimized for footprint optimization */
#define PAYLOAD_SIZE      16 /* Fixed for deterministic memory allocation */

/**
 * @struct sensor_data_t
 * @brief  Core telemetry structure with internal ring buffering.
 * @details Stores a history of payloads per sensor for asynchronous processing.
 */
typedef struct {
    uint8_t     slots[RING_BUFFER_SIZE][PAYLOAD_SIZE];  /**< History of received packets */
    uint8_t     head;                                   /**< Circular buffer index (0 to RING_BUFFER_SIZE-1) */
    uint8_t     len[RING_BUFFER_SIZE];                  /**< Individual length for each stored slot */
    uint8_t     last_payload[PAYLOAD_SIZE];             /**< Snapshot of the most recent data */
    uint8_t     last_len; 
    uint32_t    sample_count;
    TickType_t  last_rx_tick;
    bool        registered;
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