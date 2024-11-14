#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_mac.h"
#include "esp_http_server.h"
#include "esp_littlefs.h"
#include "driver/gpio.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "flowmeter.h"

// Wi-Fi configuration
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN

static const char *TAG = "wifi_softap";

// Flow calculation task handle
static TaskHandle_t flow_calc_task_handle = NULL;

// Structure to hold flow data
static struct {
    volatile uint32_t pulse_count;  // Made volatile since it's modified in ISR
    float flow_rate;
    float total_volume;
    time_t last_update;
    uint32_t last_count;
    time_t last_calc_time;
} flow_data = {
    .pulse_count = 0,
    .flow_rate = 0.0,
    .total_volume = 0.0,
    .last_update = 0,
    .last_count = 0,
    .last_calc_time = 0
};

// ISR for pulse counting
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    flow_data.pulse_count++;
}

// Initialize LittleFS
esp_err_t init_littlefs(void)
{
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "storage",
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount or format filesystem (%s)", esp_err_to_name(ret));
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_littlefs_info("storage", &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get partition information (%s)", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Mount success: total: %d, used: %d", total, used);
    return ESP_OK;
}

// HTTP file handler
static esp_err_t http_file_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];
    FILE *fd = NULL;
    struct stat file_stat;
    const char *base_path = "/littlefs";

    const char *filename = req->uri;
    if (strcmp(filename, "/") == 0) {
        filename = "/index.html";
    }

    size_t total_len = strlen(base_path) + strlen(filename) + 1;
    if (total_len > FILE_PATH_MAX) {
        ESP_LOGE(TAG, "Path too long");
        httpd_resp_send_err(req, HTTPD_414_URI_TOO_LONG, "Path too long");
        return ESP_FAIL;
    }

    // Build path safely
    strcpy(filepath, base_path);
    strcat(filepath, filename);

    if (stat(filepath, &file_stat) == -1) {
        ESP_LOGE(TAG, "Failed to stat file : %s", filepath);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File does not exist");
        return ESP_FAIL;
    }

    fd = fopen(filepath, "r");
    if (!fd) {
        ESP_LOGE(TAG, "Failed to read file : %s", filepath);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read file");
        return ESP_FAIL;
    }

    // Set content type
    if (strstr(filename, ".html")) {
        httpd_resp_set_type(req, "text/html");
    } else if (strstr(filename, ".css")) {
        httpd_resp_set_type(req, "text/css");
    } else if (strstr(filename, ".js")) {
        httpd_resp_set_type(req, "application/javascript");
    } else if (strstr(filename, ".ico")) {
        httpd_resp_set_type(req, "image/x-icon");
    }

    char *chunk = malloc(1024);
    if (chunk == NULL) {
        fclose(fd);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Out of memory");
        return ESP_FAIL;
    }

    size_t chunksize;
    do {
        chunksize = fread(chunk, 1, 1024, fd);
        if (chunksize > 0) {
            if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK) {
                free(chunk);
                fclose(fd);
                httpd_resp_sendstr_chunk(req, NULL);
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (chunksize != 0);

    free(chunk);
    fclose(fd);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// Data GET handler
static esp_err_t data_get_handler(httpd_req_t *req)
{
    char response[100];
    time_t now = time(NULL);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    char time_str[9];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", &timeinfo);
    
    snprintf(response, sizeof(response),
             "{\"flow\":%.2f,\"volume\":%.2f,\"time\":\"%s\"}",
             flow_data.flow_rate,
             flow_data.total_volume,
             time_str);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));
    
    return ESP_OK;
}

// URI handlers
static const httpd_uri_t file_server = {
    .uri       = "/*",
    .method    = HTTP_GET,
    .handler   = http_file_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t data_uri = {
    .uri       = "/data",
    .method    = HTTP_GET,
    .handler   = data_get_handler,
    .user_ctx  = NULL
};

// Start webserver
httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.stack_size = 8192; // Increased stack size for web server task

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &data_uri);
        httpd_register_uri_handler(server, &file_server);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

// Initialize pulse counter
esp_err_t init_pulse_counter(void)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << PULSE_GPIO),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    
    ESP_LOGI(TAG, "Configuring GPIO %d for pulse input", PULSE_GPIO);
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    
    esp_err_t ret = gpio_install_isr_service(0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        return ret;
    }
    
    return gpio_isr_handler_add(PULSE_GPIO, gpio_isr_handler, NULL);
}

// Calculate flow rate
static void calculate_flow_rate(void)
{
    time_t now = time(NULL);
    uint32_t current_count = flow_data.pulse_count;
    
    if (flow_data.last_calc_time > 0) {
        time_t time_diff = now - flow_data.last_calc_time;
        uint32_t pulse_diff = current_count - flow_data.last_count;
        
        if (time_diff > 0) {
            // Calculate flow rate using double for higher precision
            double flow = (pulse_diff * 60.0) / (time_diff * PULSE_CALIBRATION);
            flow_data.flow_rate = (float)flow;
            
            // Update total volume
            flow_data.total_volume += pulse_diff / PULSE_CALIBRATION;
        }
    }
    
    flow_data.last_calc_time = now;
    flow_data.last_count = current_count;
}

// Flow calculation task
static void flow_calculation_task(void *pvParameter)
{
    TickType_t last_wake_time = xTaskGetTickCount();
    
    while(1) {
        calculate_flow_rate();
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(1000));
    }
}

// WiFi event handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                             int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "Station "MACSTR" joined, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "Station "MACSTR" left, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

// Initialize WiFi in AP mode
void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                      ESP_EVENT_ANY_ID,
                                                      &wifi_event_handler,
                                                      NULL,
                                                      NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .pmf_cfg = {
                .required = true
            },
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

// Main application entry point
#ifdef CONFIG_FLOW_CALC_DEBUG
static void stack_monitor_task(void *pvParameters)
{
    while(1) {
        UBaseType_t stack_hwm = uxTaskGetStackHighWaterMark(flow_calc_task_handle);
        ESP_LOGI(TAG, "Flow calc task stack high water mark: %d words", stack_hwm);
        vTaskDelay(pdMS_TO_TICKS(10000));  // Check every 10 seconds
    }
}
#endif

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize pulse counter
    ESP_ERROR_CHECK(init_pulse_counter());

    // Initialize LittleFS
    ESP_ERROR_CHECK(init_littlefs());

    // Initialize WiFi
    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();

    // Start webserver
    start_webserver();

    // Create flow calculation task with proper stack size
    BaseType_t task_created = xTaskCreate(
        flow_calculation_task,
        "flow_calc",
        FLOW_CALC_STACK_SIZE,  // Use defined stack size
        NULL,
        FLOW_CALC_PRIORITY,    // Use defined priority
        &flow_calc_task_handle
    );

    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create flow calculation task!");
        esp_restart();
    }

    ESP_LOGI(TAG, "Flow calculation task created successfully");

    #ifdef CONFIG_FLOW_CALC_DEBUG
    // Create stack monitoring task
    xTaskCreate(
        stack_monitor_task,
        "stack_monitor",
        2048,        // Smaller stack size is fine for monitoring
        NULL,
        1,          // Lower priority than flow calculation task
        NULL
    );
    #endif
}