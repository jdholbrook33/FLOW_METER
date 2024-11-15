
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "flow_sensor.h"

static const char *TAG = "flow_sensor";

// Configuration from menuconfig
#define FLOW_SENSOR_PIN CONFIG_FLOW_SENSOR_GPIO
#define PULSES_PER_LITER CONFIG_FLOW_SENSOR_PULSES_PER_LITER
#define UPDATE_INTERVAL_MS CONFIG_FLOW_SENSOR_UPDATE_INTERVAL_MS

// Static variables for pulse counting and flow calculation
static volatile uint32_t pulse_count = 0;
static volatile uint32_t last_pulse_count = 0;
static volatile float current_flow_rate = 0.0;
static volatile float total_volume = 0.0;
static int64_t last_update_time = 0;

// ISR handler for pulse counting
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    pulse_count++;
}

// Timer callback for flow rate calculation
static void flow_rate_timer_callback(void* arg)
{
    int64_t current_time = esp_timer_get_time();
    float time_diff = (current_time - last_update_time) / 1000000.0; // Convert to seconds
    uint32_t pulses = pulse_count - last_pulse_count;
    
    // Calculate flow rate in liters per minute
    current_flow_rate = (pulses * 60.0) / (PULSES_PER_LITER * time_diff);
    
    // Update total volume
    total_volume = (float)pulse_count / PULSES_PER_LITER;
    
    // Update last values
    last_pulse_count = pulse_count;
    last_update_time = current_time;
    
    ESP_LOGD(TAG, "Flow rate: %.2f L/min, Total volume: %.2f L", current_flow_rate, total_volume);
}

esp_err_t flow_sensor_init(void)
{
    // Configure GPIO for input
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << FLOW_SENSOR_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Install GPIO ISR service
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    
    // Add ISR handler
    ESP_ERROR_CHECK(gpio_isr_handler_add(FLOW_SENSOR_PIN, gpio_isr_handler, NULL));

    // Create timer for flow rate calculation
    const esp_timer_create_args_t timer_args = {
        .callback = flow_rate_timer_callback,
        .name = "flow_rate_timer"
    };
    esp_timer_handle_t timer_handle;
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer_handle, UPDATE_INTERVAL_MS * 1000)); // Convert to microseconds

    last_update_time = esp_timer_get_time();
    ESP_LOGI(TAG, "Flow sensor initialized on GPIO%d", FLOW_SENSOR_PIN);
    
    return ESP_OK;
}

float flow_sensor_get_rate(void)
{
    return current_flow_rate;
}

float flow_sensor_get_total_volume(void)
{
    return total_volume;
}

void flow_sensor_reset_volume(void)
{
    pulse_count = 0;
    last_pulse_count = 0;
    total_volume = 0.0;
}

esp_err_t flow_sensor_deinit(void)
{
    ESP_ERROR_CHECK(gpio_isr_handler_remove(FLOW_SENSOR_PIN));
    gpio_uninstall_isr_service();
    return ESP_OK;
}