#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "rtc_manager.h"
#include <sys/time.h>

static const char *TAG = "rtc_manager";

#define I2C_MASTER_SCL_IO CONFIG_I2C_MASTER_SCL
#define I2C_MASTER_SDA_IO CONFIG_I2C_MASTER_SDA
#define I2C_MASTER_FREQ_HZ CONFIG_I2C_MASTER_FREQ_HZ
#define TIMEZONE_OFFSET CONFIG_TIMEZONE_OFFSET_HOURS

#define DS3231_ADDR 0x68 // I2C address of DS3231
#define DS3231_REG_TIME 0x00
#define DS3231_REG_TEMP 0x11

static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = 0;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t err = i2c_param_config(i2c_master_port, &conf);
    if (err != ESP_OK) {
        return err;
    }
    return i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
}

static uint8_t bcd2dec(uint8_t val)
{
    return (val >> 4) * 10 + (val & 0x0f);
}

static esp_err_t ds3231_read(uint8_t reg, uint8_t* data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS3231_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS3231_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(0, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

// In rtc_manager.c - update the init function to be more explicit about UTC->CST

esp_err_t rtc_manager_init(void)
{
    esp_err_t ret = i2c_master_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C master");
        return ret;
    }

    // Read current time from RTC (which we know is in UTC)
    uint8_t rtc_data[7];
    ret = ds3231_read(DS3231_REG_TIME, rtc_data, 7);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read RTC time");
        return ret;
    }

    // Convert BCD time values to decimal
    struct tm rtc_time = {
        .tm_sec = bcd2dec(rtc_data[0]),
        .tm_min = bcd2dec(rtc_data[1]),
        .tm_hour = bcd2dec(rtc_data[2]),
        .tm_mday = bcd2dec(rtc_data[4]),
        .tm_mon = bcd2dec(rtc_data[5] & 0x1F) - 1,  // RTC month is 1-12, tm_mon is 0-11
        .tm_year = bcd2dec(rtc_data[6]) + 100,      // RTC year is 0-99, tm_year is years since 1900
        .tm_isdst = 0
    };

    // Log the UTC time from RTC
    char utc_time_str[64];
    strftime(utc_time_str, sizeof(utc_time_str), "%Y-%m-%d %H:%M:%S UTC", &rtc_time);
    ESP_LOGI(TAG, "RTC time (UTC): %s", utc_time_str);

    // Convert to epoch time and apply CST offset (-6 hours)
    time_t epoch = mktime(&rtc_time);
    epoch += (-6 * 3600);  // Apply CST offset (hardcoded for clarity)
    
    // Set system time to CST
    struct timeval now = {
        .tv_sec = epoch,
        .tv_usec = 0,
    };
    ret = settimeofday(&now, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set system time");
        return ret;
    }

    // Log the converted CST time
    char cst_time_str[64];
    rtc_manager_get_time_str(cst_time_str, sizeof(cst_time_str));
    ESP_LOGI(TAG, "System time set to CST: %s", cst_time_str);
    
    return ESP_OK;
}

esp_err_t rtc_manager_get_time(time_t* now)
{
    if (now == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    time(now);
    return ESP_OK;
}

esp_err_t rtc_manager_get_time_str(char* buffer, size_t size)
{
    if (buffer == NULL || size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    time_t now;
    rtc_manager_get_time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S CST", &timeinfo);
    return ESP_OK;
}

// In rtc_manager.c - enhance the temperature reading function

esp_err_t rtc_manager_get_temperature(float* temp_c)
{
    if (temp_c == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t temp_data[2];
    esp_err_t ret = ds3231_read(DS3231_REG_TEMP, temp_data, 2);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read temperature");
        return ret;
    }

    // Convert the temperature data
    // First byte is integer part
    // Second byte is fractional part: bit 7,6 are decimal points (0.25°C per bit)
    *temp_c = temp_data[0] + ((temp_data[1] >> 6) * 0.25);
    
    ESP_LOGD(TAG, "Raw temp data: %02x %02x", temp_data[0], temp_data[1]);
    return ESP_OK;
}