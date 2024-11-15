
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"

// Define the configuration macros
#define WIFI_MANAGER_SSID          CONFIG_ESP_WIFI_SSID
#define WIFI_MANAGER_PASSWORD      CONFIG_ESP_WIFI_PASSWORD
#define WIFI_MANAGER_CHANNEL       CONFIG_ESP_WIFI_CHANNEL
#define WIFI_MANAGER_MAX_CONN      CONFIG_ESP_MAX_STA_CONN

/**
 * @brief Initialize WiFi in softAP mode using menuconfig settings
 * 
 * @return esp_err_t ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t wifi_manager_init_softap(void);

/**
 * @brief Deinitialize WiFi manager
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t wifi_manager_deinit(void);

#endif // WIFI_MANAGER_H
