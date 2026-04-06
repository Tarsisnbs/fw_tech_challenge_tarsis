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
    uint8_t     last_len;                               /**< Length of the latest snapshot */
    uint32_t    sample_count;                           /**< Total valid packets processed since boot */
    TickType_t  last_rx_tick;                           /**< System tick of the last successful update */
    bool        registered;                             /**< Connectivity flag for the watchdog monitor */
} sensor_data_t;

/* --- Public API --- */

/** @brief Initializes the storage mutex and clears all sensor records. */
void storage_init(void);

/** @brief Persists a new packet from the Parser into the sensor's circular buffer. */
bool storage_save_packet(uint8_t sensor_id, uint8_t *payload, uint8_t len);

/** * @brief  Retrieves a pointer to a specific sensor's data.
 * @note   CRITICAL: Must be called between storage_lock() and storage_unlock().
 */
sensor_data_t* storage_get_sensor(uint8_t sensor_id);

/** @brief Attempts to acquire the storage mutex for thread-safe access. */
bool storage_lock(TickType_t xTicksToWait);
/** @brief Releases the storage mutex. */
void storage_unlock(void);

#endif