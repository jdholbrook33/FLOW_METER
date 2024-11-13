#ifndef FLOWMETER_H
#define FLOWMETER_H

#include "esp_err.h"
#include "esp_http_server.h"

// Configuration defines
#define FILE_PATH_MAX 512
#define PULSE_GPIO GPIO_NUM_3        // Using GPIO 3 for pulse input
#define PULSE_CALIBRATION 300.0      // 300 pulses per liter
#define FLOW_CALC_STACK_SIZE (4096)  // Stack size for flow calculation task
#define FLOW_CALC_PRIORITY (5)       // Priority for flow calculation task

// Function declarations
void wifi_init_softap(void);
esp_err_t init_littlefs(void);
httpd_handle_t start_webserver(void);
esp_err_t init_pulse_counter(void);

#endif // FLOWMETER_H