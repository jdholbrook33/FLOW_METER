#include <string.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "web_server.h"
#include <errno.h>
#include <sys/stat.h>  

static const char *TAG = "web_server";
static httpd_handle_t server = NULL;

#define FILE_PATH_MAX 255

// MIME type structure
typedef struct {
    const char *extension;
    const char *mime_type;
} mime_type_t;

// MIME type mapping
static const mime_type_t mime_types[] = {
    {".html", "text/html"},
    {".css",  "text/css"},
    {".js",   "application/javascript"},
    {".json", "application/json"},
    {".png",  "image/png"},
    {".jpg",  "image/jpeg"},
    {".ico",  "image/x-icon"},
    {".svg",  "image/svg+xml"},
    {NULL,    "text/plain"}  // Default MIME type
};

// Get MIME type for file
static const char* get_mime_type(const char *filepath) {
    const char *ext = strrchr(filepath, '.');
    if (ext) {
        for (const mime_type_t *mime = mime_types; mime->extension; mime++) {
            if (strcasecmp(ext, mime->extension) == 0) {
                return mime->mime_type;
            }
        }
    }
    return "text/plain";
}

// HTTP file handler
static esp_err_t http_file_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];
    FILE *fd = NULL;
    struct stat file_stat;
    const char *base_path = "/littlefs";
    const char *filename = req->uri;

    ESP_LOGI(TAG, "Received request for URI: %s", req->uri);  // Add this

    if (strcmp(filename, "/") == 0) {
        filename = "/index.html";
    }

    ESP_LOGI(TAG, "Looking for file: %s", filename);  // Add this

    size_t total_len = strlen(base_path) + strlen(filename) + 1;
    if (total_len > FILE_PATH_MAX) {
        ESP_LOGE(TAG, "Path too long");
        httpd_resp_send_err(req, HTTPD_414_URI_TOO_LONG, "Path too long");
        return ESP_FAIL;
    }

    // Build path safely
    strcpy(filepath, base_path);
    strcat(filepath, filename);

    ESP_LOGI(TAG, "Full filepath: %s", filepath);  // Add this

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


esp_err_t web_server_init(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    config.max_uri_handlers = 8;

    ESP_LOGI(TAG, "Starting HTTP server on port %d", config.server_port);
    
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return ESP_FAIL;
    }

    // Register root path first
    httpd_uri_t root = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = http_file_handler,
        .user_ctx  = NULL
    };

    ESP_LOGI(TAG, "Registering root (/) handler");
    esp_err_t ret = httpd_register_uri_handler(server, &root);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register root handler: %d", ret);
        return ret;
    }

    // Register catch-all handler
    httpd_uri_t file_download = {
        .uri       = "/*",
        .method    = HTTP_GET,
        .handler   = http_file_handler,
        .user_ctx  = NULL
    };

    ESP_LOGI(TAG, "Registering catch-all handler");
    ret = httpd_register_uri_handler(server, &file_download);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register catch-all handler: %d", ret);
        return ret;
    }

    ESP_LOGI(TAG, "HTTP server started successfully");
    return ESP_OK;
}