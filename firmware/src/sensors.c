#include "sensors.h"
#include "storage.h"
#include "FreeRTOS.h"
#include "task.h"

/**
 * @file    sensors.c
 * @brief   Sensor Health and Connectivity Management.
 * @details This module interfaces with the Storage layer to track sensor 
 * registration and communication timestamps for fault detection.
 */

/**
 * @brief  Updates the "last seen" timestamp and registration for a sensor.
 * @details This function is called by the Protocol Parser. It marks the 
 * sensor as active and stores the current system tick for watchdog checks.
 * @param  sensor_id: Unique identifier for the sensor node.
 * @note   Uses a 10ms Mutex timeout to prevent blocking the Parser task 
 * if the storage is busy.
 */

void sensors_touch_alive(uint8_t sensor_id) {
    /* Attempt to acquire storage lock to ensure atomic update of sensor state */
    if (storage_lock(pdMS_TO_TICKS(10))) {
        sensor_data_t* s = storage_get_sensor(sensor_id);
        if (s) {
            s->registered = true;
            s->last_rx_tick = xTaskGetTickCount();
        }
        /* Release resource for other tasks */
        storage_unlock();
    }
}

/**
 * @brief  Scans all registered sensors for communication timeouts.
 * @details Iterates through the storage registry to identify sensors 
 * that haven't reported within the allowed window.
 * @param  timeout_ms: Maximum allowed silence period in milliseconds.
 * @return true if ANY registered sensor is timed out; false otherwise.
 * @note   This check is the primary source for the System Fault (PA6) indicator.
 */
bool sensors_check_for_timeout(uint32_t timeout_ms) {
    bool fault = false;
    TickType_t now = xTaskGetTickCount();
    TickType_t limit = pdMS_TO_TICKS(timeout_ms);

    /* Lock storage with 20ms window to perform a consistent scan of all nodes */
    if (storage_lock(pdMS_TO_TICKS(20))) {
        for (int i = 0; i < MAX_SENSORS; i++) {
            sensor_data_t* s = storage_get_sensor(i);

            /* Logic: Only check sensors that have been previously registered */
            if (s && s->registered) {
                /* Calculate elapsed time since last valid packet */
                if ((now - s->last_rx_tick) > limit) {
                    fault = true;
                    break; /* Fail-fast: One faulty sensor triggers a system fault */
                }
            }
        }
        storage_unlock();
    }
    return fault;
}