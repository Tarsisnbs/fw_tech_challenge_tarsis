#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @file    sensors.h
 * @brief   Sensor Health and Connectivity Monitoring.
 * @details Provides an interface to track sensor activity and detect 
 * communication timeouts across the system.
 */

/* --- Public Functions --- */

/**
 * @brief  Initialize the sensor control structures.
 * @details Clears registration flags and resets internal timers for all 
 * potential sensor nodes.
 */
void sensors_init(void);

/**
 * @brief  Notify the system that a sensor is active.
 * @details Updates the "last seen" timestamp for a specific sensor ID. 
 * Should be called whenever a valid packet is successfully parsed.
 * @param  sensor_id: The unique identifier of the reporting sensor.
 */
void sensors_touch_alive(uint8_t sensor_id);

/**
 * @brief  Check all registered sensors for connectivity timeouts.
 * @details Performs a scan across the sensor registry to verify if any 
 * node has exceeded the allowed silence period.
 * @param  timeout_ms: The maximum allowed time (in ms) since the last update.
 * @return true if a timeout is detected in ANY registered sensor, false otherwise.
 * @note   This function is used by the Fault Monitor task to trigger system alarms.
 */
bool sensors_check_for_timeout(uint32_t timeout_ms);

#endif