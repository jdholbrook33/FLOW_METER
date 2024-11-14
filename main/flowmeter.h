#ifndef FLOWMETER_H
#define FLOWMETER_H

#include "esp_err.h"
#include "esp_http_server.h"
#include "driver/i2c_master.h"

// Configuration defines
#define FILE_PATH_MAX 512
#define PULSE_GPIO GPIO_NUM_3        // Using GPIO 3 for pulse input
#define PULSE_CALIBRATION 300.0      // 300 pulses per liter
#define FLOW_CALC_STACK_SIZE (4096)  // Stack size for flow calculation task
#define FLOW_CALC_PRIORITY (5)       // Priority for flow calculation task

// Note: ESP32-C3 specific GPIO configurations
#define I2C_MASTER_SCL_IO 7         // Default I2C pins for C3
#define I2C_MASTER_SDA_IO 6         // Default I2C pins for C3
#define I2C_MASTER_NUM    0        // I2C master i2c port number
#define I2C_MASTER_FREQ_HZ 100000
#define I2C_MASTER_TIMEOUT_MS 1000
#define DS3231_ADDR 0x68

// Function declarations
void wifi_init_softap(void);
esp_err_t init_littlefs(void);
httpd_handle_t start_webserver(void);
esp_err_t init_pulse_counter(void);
esp_err_t init_i2c(void);
esp_err_t sync_rtc_from_ds3231(void);

#endif // FLOWMETER_H