#ifndef RTC_MANAGER_H
#define RTC_MANAGER_H

#include "esp_err.h"
#include <time.h>

/**
 * @brief Initialize the RTC manager
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t rtc_manager_init(void);

/**
 * @brief Get current time as a time_t value (seconds since epoch, in CST)
 *
 * @param[out] now Pointer to store the current time
 * @return esp_err_t ESP_OK on success
 */
esp_err_t rtc_manager_get_time(time_t* now);

/**
 * @brief Get formatted time string (in CST)
 *
 * @param[out] buffer Buffer to store the formatted time
 * @param size Buffer size
 * @return esp_err_t ESP_OK on success
 */
esp_err_t rtc_manager_get_time_str(char* buffer, size_t size);

/**
 * @brief Get RTC temperature (DS3231 specific feature)
 *
 * @param[out] temp_c Temperature in Celsius
 * @return esp_err_t ESP_OK on success
 */
esp_err_t rtc_manager_get_temperature(float* temp_c);

#endif // RTC_MANAGER_H