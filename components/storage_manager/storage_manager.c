#include <string.h>
#include <unistd.h>  
#include "esp_log.h"
#include "esp_littlefs.h"
#include "storage_manager.h"

static const char *TAG = "storage_manager";

esp_err_t storage_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing LittleFS");

    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "storage",
        .format_if_mount_failed = true,
        .dont_mount = false,
    };

    // Mount LittleFS
    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_littlefs_info("storage", &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get LittleFS partition information");
        return ret;
    }

    ESP_LOGI(TAG, "LittleFS mounted successfully. Total: %d bytes, Used: %d bytes", 
             total, used);
    return ESP_OK;
}

esp_err_t storage_manager_write_file(const char *path, const char *data, size_t len)
{
    char full_path[128];
    snprintf(full_path, sizeof(full_path), "/littlefs/%s", path);

    ESP_LOGI(TAG, "Writing file: %s", full_path);

    FILE *f = fopen(full_path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }

    size_t written = fwrite(data, 1, len, f);
    fclose(f);

    if (written != len) {
        ESP_LOGE(TAG, "Failed to write complete file");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "File written successfully");
    return ESP_OK;
}

esp_err_t storage_manager_read_file(const char *path, char *data, size_t max_len, size_t *actual_len)
{
    char full_path[128];
    snprintf(full_path, sizeof(full_path), "/littlefs/%s", path);

    ESP_LOGI(TAG, "Reading file: %s", full_path);

    FILE *f = fopen(full_path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }

    *actual_len = fread(data, 1, max_len - 1, f);
    data[*actual_len] = '\0';  // Null terminate the string
    fclose(f);

    ESP_LOGI(TAG, "File read successfully");
    return ESP_OK;
}

esp_err_t storage_manager_delete_file(const char *path)
{
    char full_path[128];
    snprintf(full_path, sizeof(full_path), "/littlefs/%s", path);

    ESP_LOGI(TAG, "Deleting file: %s", full_path);

    int ret = unlink(full_path);
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to delete file");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "File deleted successfully");
    return ESP_OK;
}

esp_err_t storage_manager_get_stats(size_t *total_bytes, size_t *used_bytes)
{
    return esp_littlefs_info("storage", total_bytes, used_bytes);
}
