#ifndef FLOW_SENSOR_H
#define FLOW_SENSOR_H

#include <stdint.h>
#include "esp_err.h"

/**
 * @brief Initialize the flow sensor
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t flow_sensor_init(void);

/**
 * @brief Get the current flow rate in liters per minute
 *
 * @return float Current flow rate
 */
float flow_sensor_get_rate(void);

/**
 * @brief Get the total volume in liters since last reset
 *
 * @return float Total volume
 */
float flow_sensor_get_total_volume(void);

/**
 * @brief Reset the total volume counter
 */
void flow_sensor_reset_volume(void);

/**
 * @brief Deinitialize the flow sensor
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t flow_sensor_deinit(void);

#endif // FLOW_SENSOR_H