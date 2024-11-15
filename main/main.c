
#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_log.h"
#include "wifi_manager.h"
#include "flow_sensor.h"
#include "rtc_manager.h"
#include "storage_manager.h"
#include "web_server.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "main";

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize WiFi in AP mode using menuconfig settings
    ESP_LOGI(TAG, "Initializing WiFi in AP mode");
    wifi_manager_init_softap();

    // Initialize RTC and sync system time
    ESP_ERROR_CHECK(rtc_manager_init());
    
    // Initialize flow sensor
    ESP_ERROR_CHECK(flow_sensor_init());

        // Initialize storage
    ESP_ERROR_CHECK(storage_manager_init());


    // Initialize web server
    ESP_ERROR_CHECK(web_server_init());


    // Get storage stats
    size_t total_bytes, used_bytes;
    ESP_ERROR_CHECK(storage_manager_get_stats(&total_bytes, &used_bytes));
    ESP_LOGI(TAG, "Storage stats - Total: %d bytes, Used: %d bytes", total_bytes, used_bytes);

    
    // Main loop: print time and flow readings
while (1) {
    char time_str[64];
    rtc_manager_get_time_str(time_str, sizeof(time_str));
    float rate = flow_sensor_get_rate();
    float volume = flow_sensor_get_total_volume();
    float temperature;
    rtc_manager_get_temperature(&temperature);
    
    ESP_LOGI(TAG, "[%s] Flow rate: %.2f L/min, Total: %.2f L, Temp: %.2f°C",
             time_str, rate, volume, temperature);
             
    vTaskDelay(pdMS_TO_TICKS(1000));
}
}