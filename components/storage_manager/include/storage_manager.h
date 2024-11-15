#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include "esp_err.h"

/**
 * @brief Initialize and mount the LittleFS filesystem
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t storage_manager_init(void);

/**
 * @brief Write a file to LittleFS
 * 
 * @param path File path
 * @param data Data to write
 * @param len Length of data
 * @return esp_err_t ESP_OK on success
 */
esp_err_t storage_manager_write_file(const char *path, const char *data, size_t len);

/**
 * @brief Read a file from LittleFS
 * 
 * @param path File path
 * @param data Buffer to store data
 * @param max_len Maximum length to read
 * @param actual_len Actual length read
 * @return esp_err_t ESP_OK on success
 */
esp_err_t storage_manager_read_file(const char *path, char *data, size_t max_len, size_t *actual_len);

/**
 * @brief Delete a file from LittleFS
 * 
 * @param path File path
 * @return esp_err_t ESP_OK on success
 */
esp_err_t storage_manager_delete_file(const char *path);

/**
 * @brief Get filesystem statistics
 * 
 * @param total_bytes Total size of filesystem
 * @param used_bytes Used space in filesystem
 * @return esp_err_t ESP_OK on success
 */
esp_err_t storage_manager_get_stats(size_t *total_bytes, size_t *used_bytes);

#endif // STORAGE_MANAGER_H