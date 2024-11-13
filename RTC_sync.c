/*RTC Module setting code.
This code uses wifi to hit the NST (time.windows.com) and set the RTC module*/
#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_sntp.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "TIME_SYNC"

// I2C configuration
#define I2C_MASTER_SDA_IO 6
#define I2C_MASTER_SCL_IO 7
#define I2C_MASTER_FREQ_HZ 100000
#define I2C_MASTER_NUM 0
#define DS3231_ADDR 0x68

// WiFi credentials - replace with yours
#define WIFI_SSID "HOLBROOK24"
#define WIFI_PASS "11311jones"

static bool wifi_connected = false;
static bool time_synchronized = false;

// Convert decimal to BCD
static uint8_t dec_to_bcd(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

// Initialize I2C
static esp_err_t init_i2c(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ
    };

    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) return err;

    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

// Set time in RTC
static esp_err_t set_rtc_time(struct tm *timeinfo) {
    uint8_t data[7];
    
    // Convert time components to BCD
    data[0] = dec_to_bcd(timeinfo->tm_sec);
    data[1] = dec_to_bcd(timeinfo->tm_min);
    data[2] = dec_to_bcd(timeinfo->tm_hour);
    data[3] = dec_to_bcd(timeinfo->tm_wday + 1);
    data[4] = dec_to_bcd(timeinfo->tm_mday);
    data[5] = dec_to_bcd(timeinfo->tm_mon + 1);
    data[6] = dec_to_bcd((timeinfo->tm_year + 1900) % 100);

    // Write time to RTC
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS3231_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true);  // Start at register 0
    i2c_master_write(cmd, data, 7, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    return ret;
}

// WiFi event handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                             int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Connecting to WiFi...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected. Retrying...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        wifi_connected = true;
    }
}

// Time sync callback
static void time_sync_cb(struct timeval *tv) {
    time_t now = 0;
    struct tm timeinfo = { 0 };
    
    time(&now);
    localtime_r(&now, &timeinfo);

    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "NTP time synchronized: %s", strftime_buf);

    // Set RTC time
    if (set_rtc_time(&timeinfo) == ESP_OK) {
        ESP_LOGI(TAG, "RTC time set successfully");
    } else {
        ESP_LOGE(TAG, "Failed to set RTC time");
    }
    
    time_synchronized = true;
}

// Initialize WiFi
static void init_wifi(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, 
                                             &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, 
                                             &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

// Initialize and configure SNTP
static void init_sntp(void) {
    ESP_LOGI(TAG, "Initializing SNTP");
    
    // Set timezone to EST
    setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
    tzset();
    
    // Initialize SNTP
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_set_time_sync_notification_cb(time_sync_cb);
    esp_sntp_init();
}

void app_main(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize I2C
    ESP_ERROR_CHECK(init_i2c());

    // Initialize WiFi
    init_wifi();

    // Wait for WiFi connection
    ESP_LOGI(TAG, "Waiting for WiFi connection...");
    while (!wifi_connected) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Initialize and start SNTP
    init_sntp();

    // Wait for time sync
    while (!time_synchronized) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Disconnect WiFi after time sync
    ESP_LOGI(TAG, "Time synchronized, disconnecting WiFi");
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    ESP_ERROR_CHECK(esp_wifi_stop());
}