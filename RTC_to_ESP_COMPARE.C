#include <stdio.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <time.h>

#define TAG "RTC_MONITOR"

// I2C configuration
#define I2C_MASTER_SCL_IO 6          // SCL pin
#define I2C_MASTER_SDA_IO 5          // SDA pin
#define I2C_MASTER_FREQ_HZ 100000    // 100kHz I2C clock
#define I2C_MASTER_NUM 0             // I2C port number
#define DS3231_ADDR 0x68             // RTC I2C address

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

// Convert BCD to decimal
static uint8_t bcd_to_dec(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

// Read time from RTC
static esp_err_t read_rtc_time(struct tm *timeinfo) {
    uint8_t data[7];
    
    // Read time registers from RTC
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS3231_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true);  // Start at register 0
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS3231_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, 7, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

// In the read_rtc_time function:
if (ret == ESP_OK) {
    // Add debug output before conversion
    ESP_LOGI(TAG, "Raw RTC registers: %02x %02x %02x %02x %02x %02x %02x",
             data[0], data[1], data[2], data[3], data[4], data[5], data[6]);
    
    // Then our normal conversion
    timeinfo->tm_sec = bcd_to_dec(data[0] & 0x7F);
    timeinfo->tm_min = bcd_to_dec(data[1] & 0x7F);
    timeinfo->tm_hour = bcd_to_dec(data[2] & 0x3F);
    timeinfo->tm_wday = bcd_to_dec(data[3] & 0x07);
    timeinfo->tm_mday = bcd_to_dec(data[4] & 0x3F);
    timeinfo->tm_mon = bcd_to_dec(data[5] & 0x1F) - 1;  // Convert from 1-12 to 0-11 range
    timeinfo->tm_year = bcd_to_dec(data[6]) + 100;
}

    return ret;
}

// Format and print time
static void print_time(const char* source, const struct tm *timeinfo) {
    char timestr[64];
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", timeinfo);
    ESP_LOGI(TAG, "%s: %s", source, timestr);
}

void app_main(void) {
    // Initialize I2C
    ESP_ERROR_CHECK(init_i2c());
    ESP_LOGI(TAG, "I2C initialized successfully");

    struct tm rtc_time;
    time_t now;
    struct tm esp_time;

    // Monitor loop
    while(1) {
        // Read RTC time
        if (read_rtc_time(&rtc_time) == ESP_OK) {
            print_time("RTC", &rtc_time);
        } else {
            ESP_LOGE(TAG, "Failed to read RTC time");
        }

        // Read ESP32 internal time
        time(&now);
        localtime_r(&now, &esp_time);
        print_time("ESP", &esp_time);

        ESP_LOGI(TAG, "------------------------");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}