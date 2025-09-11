#include "esp_http_server.h"
#include "esp_log.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

#include "esp_spiffs.h"
#include "esp_vfs.h"
#include "driver/uart.h"

#include "http_server.h"
#include "tasks_common.h"
#include "wifi_app.h"

static const char TAG[] = "http_server";
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// http server task handle
static httpd_handle_t http_server_handle = NULL;

// http server task handle
static TaskHandle_t task_http_server_monitor = NULL;

// htt server monitor task queue
static QueueHandle_t http_server_monitor_queue_handle = NULL;

// Embedded files: index.html, style.css, script.js and favicon.ico files
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
extern const uint8_t script_js_start[] asm("_binary_script_js_start");
extern const uint8_t script_js_end[] asm("_binary_script_js_end");
extern const uint8_t style_css_start[] asm("_binary_style_css_start");
extern const uint8_t style_css_end[] asm("_binary_style_css_end");

// UART configuration for communication with STM32
#define UART_PORT_NUM UART_NUM_1
#define UART_TX_BUFFER_SIZE 1024
#define UART_RX_BUFFER_SIZE 1024

// Protocol Commands for ESP32-STM32 Communication
#define FW_REQUEST 28
#define FW_LENGTH 2
#define CHECKSUM_DATA 6

// Protocol Responses from STM32
#define FW_READY 31
#define FW_ERR 4
#define FW_OK 3
#define FW_RECEIVED 5
#define CHECKSUM_OK 7
#define CHECKSUM_ERR 8

// Protocol Settings
#define PROTOCOL_TIMEOUT_MS 10000 // Increase to 10 seconds for STM32 processing time
#define DATA_CHUNK_SIZE 8         // 8 bytes per chunk for better performance
#define MAX_RESPONSE_SIZE 32      // Maximum response string size

// Firmware file paths in SPIFFS
#define FIRMWARE_FILE_PATH "/spiffs/firmware.bin"
#define TEMP_HEX_FILE_PATH "/spiffs/temp.hex"

// Function to initialize SPIFFS
esp_err_t init_spiffs(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL, // Use default partition label "spiffs"
        .max_files = 5,
        .format_if_mount_failed = true};
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI(TAG, "Partition size: total:%d, used:%d", total, used);
    }
    return ESP_OK;
}

// Function to convert Intel HEX to binary
esp_err_t convert_hex_to_bin(const char *hex_file_path, const char *bin_file_path)
{
    FILE *hex_file = fopen(hex_file_path, "r");
    if (hex_file == NULL)
    {
        ESP_LOGE(TAG, "Failed to open HEX file: %s", hex_file_path);
        return ESP_FAIL;
    }

    char line[256];
    uint32_t min_address = 0xFFFFFFFF;
    uint32_t max_address = 0;
    uint32_t base_address = 0;
    bool first_data_record = true;

    ESP_LOGI(TAG, "Converting HEX to BIN...");

    // First pass: find the address range
    while (fgets(line, sizeof(line), hex_file))
    {
        if (line[0] != ':')
            continue;

        // Parse Intel HEX record
        char byte_count_str[3] = {line[1], line[2], '\0'};
        char address_str[5] = {line[3], line[4], line[5], line[6], '\0'};
        char record_type_str[3] = {line[7], line[8], '\0'};

        uint8_t byte_count = (uint8_t)strtol(byte_count_str, NULL, 16);
        uint16_t address = (uint16_t)strtol(address_str, NULL, 16);
        uint8_t record_type = (uint8_t)strtol(record_type_str, NULL, 16);

        if (record_type == 0x00)
        { // Data record
            uint32_t full_address = base_address + address;
            uint32_t end_address = full_address + byte_count;

            if (first_data_record)
            {
                min_address = full_address;
                first_data_record = false;
            }

            if (full_address < min_address)
            {
                min_address = full_address;
            }
            if (end_address > max_address)
            {
                max_address = end_address;
            }
        }
        else if (record_type == 0x04)
        { // Extended Linear Address
            char ext_addr_str[5] = {line[9], line[10], line[11], line[12], '\0'};
            base_address = ((uint32_t)strtol(ext_addr_str, NULL, 16)) << 16;
        }
        else if (record_type == 0x01)
        { // End of File
            break;
        }
    }

    if (first_data_record)
    {
        ESP_LOGE(TAG, "No data records found in HEX file");
        fclose(hex_file);
        return ESP_FAIL;
    }

    uint32_t binary_size = max_address - min_address;
    ESP_LOGI(TAG, "Address range: 0x%08lX to 0x%08lX", min_address, max_address);
    ESP_LOGI(TAG, "Binary size will be: %lu bytes", binary_size);

    // Allocate memory for binary data
    uint8_t *binary_data = malloc(binary_size);
    if (binary_data == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for binary data");
        fclose(hex_file);
        return ESP_FAIL;
    }

    // Initialize with 0xFF (typical for flash memory)
    memset(binary_data, 0xFF, binary_size);

    // Second pass: convert data
    fseek(hex_file, 0, SEEK_SET);
    base_address = 0;

    while (fgets(line, sizeof(line), hex_file))
    {
        if (line[0] != ':')
            continue;

        // Parse Intel HEX record
        char byte_count_str[3] = {line[1], line[2], '\0'};
        char address_str[5] = {line[3], line[4], line[5], line[6], '\0'};
        char record_type_str[3] = {line[7], line[8], '\0'};

        uint8_t byte_count = (uint8_t)strtol(byte_count_str, NULL, 16);
        uint16_t address = (uint16_t)strtol(address_str, NULL, 16);
        uint8_t record_type = (uint8_t)strtol(record_type_str, NULL, 16);

        if (record_type == 0x00)
        { // Data record
            uint32_t full_address = base_address + address;
            uint32_t offset = full_address - min_address;

            // Extract data bytes
            for (int i = 0; i < byte_count; i++)
            {
                char byte_str[3] = {line[9 + i * 2], line[10 + i * 2], '\0'};
                uint8_t data_byte = (uint8_t)strtol(byte_str, NULL, 16);

                if (offset + i < binary_size)
                {
                    binary_data[offset + i] = data_byte;
                }
            }
        }
        else if (record_type == 0x04)
        { // Extended Linear Address
            char ext_addr_str[5] = {line[9], line[10], line[11], line[12], '\0'};
            base_address = ((uint32_t)strtol(ext_addr_str, NULL, 16)) << 16;
        }
        else if (record_type == 0x01)
        { // End of File
            break;
        }
    }

    fclose(hex_file);

    // Write binary data to file
    FILE *bin_file = fopen(bin_file_path, "wb");
    if (bin_file == NULL)
    {
        ESP_LOGE(TAG, "Failed to create BIN file: %s", bin_file_path);
        free(binary_data);
        return ESP_FAIL;
    }

    size_t written = fwrite(binary_data, 1, binary_size, bin_file);
    fclose(bin_file);
    free(binary_data);

    if (written != binary_size)
    {
        ESP_LOGE(TAG, "Failed to write complete binary file");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "HEX to BIN conversion completed. Binary size: %lu bytes", binary_size);
    return ESP_OK;
}

// Protocol helper functions for byte-based communication
static esp_err_t send_command_byte(uint8_t command) {
    int bytes_written = uart_write_bytes(UART_PORT_NUM, &command, 1);
    if (bytes_written != 1) {
        ESP_LOGE(TAG, "Failed to send command: %d", command);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Sent command: %d", command);
    return ESP_OK;
}

static esp_err_t send_command_with_data(uint8_t command, uint32_t data) {
    uint8_t packet[5];
    packet[0] = command;
    packet[1] = (data >> 24) & 0xFF;  // Big-endian format
    packet[2] = (data >> 16) & 0xFF;
    packet[3] = (data >> 8) & 0xFF;
    packet[4] = data & 0xFF;
    
    int bytes_written = uart_write_bytes(UART_PORT_NUM, packet, 5);
    if (bytes_written != 5) {
        ESP_LOGE(TAG, "Failed to send command with data: %d, data: %lu", command, data);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Sent command: %d with data: %lu", command, data);
    return ESP_OK;
}

static esp_err_t wait_for_response_byte(uint8_t expected_response, uint32_t timeout_ms) {
    TickType_t start_time = xTaskGetTickCount();
    TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);

    while ((xTaskGetTickCount() - start_time) < timeout_ticks) {
        uint8_t byte;
        int bytes_read = uart_read_bytes(UART_PORT_NUM, &byte, 1, pdMS_TO_TICKS(100));

        if (bytes_read > 0) {
            if (byte == expected_response) {
                ESP_LOGI(TAG, "Received expected response: %d", expected_response);
                return ESP_OK;
            } else {
                ESP_LOGW(TAG, "Received unexpected response: %d (expected: %d)", byte, expected_response);
            }
        }
    }

    ESP_LOGE(TAG, "Timeout waiting for response: %d", expected_response);
    return ESP_ERR_TIMEOUT;
}

// Helper function to wait for either FW_OK or FW_ERR response
static esp_err_t wait_for_fw_ok_or_err(uint32_t timeout_ms) {
    TickType_t start_time = xTaskGetTickCount();
    TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);

    while ((xTaskGetTickCount() - start_time) < timeout_ticks) {
        uint8_t byte;
        int bytes_read = uart_read_bytes(UART_PORT_NUM, &byte, 1, pdMS_TO_TICKS(100));

        if (bytes_read > 0) {
            if (byte == FW_OK) {
                ESP_LOGI(TAG, "Received FW_OK - STM32 accepted the length");
                return ESP_OK;
            } else if (byte == FW_ERR) {
                ESP_LOGE(TAG, "Received FW_ERR - STM32 rejected the length");
                return ESP_FAIL;
            } else {
                ESP_LOGW(TAG, "Received unexpected response: %d (expected: FW_OK=%d or FW_ERR=%d)", 
                         byte, FW_OK, FW_ERR);
            }
        }
    }

    ESP_LOGE(TAG, "Timeout waiting for FW_OK or FW_ERR response");
    return ESP_ERR_TIMEOUT;
}

//Simple sum algorithm - compatible with STM32
static uint32_t calculate_checksum(const uint8_t *data, size_t size)
{
    uint32_t checksum = 0;
    for (size_t i = 0; i < size; i++)
    {
        checksum += data[i];
    }
    return checksum % 256;
}

// Function to initialize UART for STM32 communication
void init_uart(void)
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};
    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_set_pin(UART_PORT_NUM, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE); // TX GPIO17, RX GPIO16 - adjust as needed
    uart_driver_install(UART_PORT_NUM, UART_RX_BUFFER_SIZE * 2, UART_TX_BUFFER_SIZE * 2, 0, NULL, 0);
    ESP_LOGI(TAG, "UART initialized for STM32 communication");
}

static void http_server_monitor(void *arg)
{
    http_server_queue_message_t msg;
    while (1)
    {
        if (xQueueReceive(http_server_monitor_queue_handle, &msg, portMAX_DELAY))
        {
            switch (msg.msgID)
            {
            case HTTP_MSG_WIFI_CONNECT_INIT:
                ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_INIT");
                break;
            case HTTP_MSG_WIFI_CONNECT_SUCCESS:
                ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_SUCCESS");
                break;
            case HTTP_MSG_WIFI_CONNECT_FAIL:
                ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_FAIL");
                break;
            case HTTP_MSG_OTA_UPDATE_SUCCESSFULL:
                ESP_LOGI(TAG, "HTTP_MSG_OTA_UPDATE_SUCCESSFULL");
                break;
            case HTTP_MSG_OTA_UPDATE_FAILED:
                ESP_LOGI(TAG, "HTTP_MSG_OTA_UPDATE_FAILED");
                break;
            case HTTP_MSG_OTA_UPDATE_INITIALIZED:
                ESP_LOGI(TAG, "HTTP_MSG_OTA_UPDATE_INITIALIZED");
                break;
            default:
                break;
            }
        }
    }
}

static esp_err_t http_server_index_html_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "index.html requested");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start,
                    index_html_end - index_html_start);
    return ESP_OK;
}

static esp_err_t http_server_script_js_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "script.js requested");
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, (const char *)script_js_start,
                    script_js_end - script_js_start);
    return ESP_OK;
}

static esp_err_t http_server_style_css_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "style.css requested");
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char *)style_css_start,
                    style_css_end - style_css_start);
    return ESP_OK;
}
// Handler for firmware upload (/upload POST)
static esp_err_t http_server_upload_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Firmware upload started");

    // Check content length
    if (req->content_len <= 0)
    {
        ESP_LOGE(TAG, "No content received");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No content received");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Content length: %d bytes", req->content_len);

    // Send OTA update initialized message
    http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_INITIALIZED);

    // Buffer for reading headers and finding content start
    char header_buffer[2048];
    int header_received = 0;
    char *content_start_marker = "\r\n\r\n";
    int content_start_pos = 0;

    // Read initial data to find headers
    int recv_len = httpd_req_recv(req, header_buffer, sizeof(header_buffer) - 1);
    if (recv_len <= 0)
    {
        ESP_LOGE(TAG, "Error receiving initial data");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Error receiving data");
        return ESP_FAIL;
    }

    header_buffer[recv_len] = '\0';
    header_received = recv_len;

    // Find content start position
    char *content_start = strstr(header_buffer, content_start_marker);
    if (content_start != NULL)
    {
        content_start_pos = (content_start - header_buffer) + 4; // Skip \r\n\r\n
        ESP_LOGI(TAG, "Content starts at position: %d", content_start_pos);
    }
    else
    {
        ESP_LOGE(TAG, "Content start not found in headers");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid multipart format");
        return ESP_FAIL;
    }

    // Open file for writing
    FILE *file = fopen(FIRMWARE_FILE_PATH, "wb");
    if (file == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file for writing");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to open file");
        return ESP_FAIL;
    }

    // Write any content that was already received after headers
    int content_in_first_chunk = header_received - content_start_pos;
    if (content_in_first_chunk > 0)
    {
        fwrite(header_buffer + content_start_pos, 1, content_in_first_chunk, file);
        ESP_LOGI(TAG, "Wrote %d bytes from first chunk", content_in_first_chunk);
    }

    // Continue receiving and writing data
    char buffer[1024];
    int total_received = header_received;
    int remaining = req->content_len - header_received;

    while (remaining > 0)
    {
        recv_len = httpd_req_recv(req, buffer, MIN(remaining, sizeof(buffer)));
        if (recv_len <= 0)
        {
            if (recv_len == HTTPD_SOCK_ERR_TIMEOUT)
            {
                ESP_LOGW(TAG, "Socket timeout, continuing...");
                continue;
            }
            ESP_LOGE(TAG, "Error receiving data: %d", recv_len);
            fclose(file);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Error receiving data");
            return ESP_FAIL;
        }

        // Write to file
        fwrite(buffer, 1, recv_len, file);

        total_received += recv_len;
        remaining -= recv_len;

        // Log progress
        int progress = (total_received * 100) / req->content_len;
        ESP_LOGI(TAG, "Upload progress: %d%% (%d/%d bytes)", progress, total_received, req->content_len);
    }

    fclose(file);

    ESP_LOGI(TAG, "Raw upload completed. Now cleaning up multipart boundaries...");

    // Now clean up the file by removing multipart boundaries
    FILE *read_file = fopen(FIRMWARE_FILE_PATH, "rb");
    if (read_file == NULL)
    {
        ESP_LOGE(TAG, "Failed to reopen file for cleanup");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "File cleanup failed");
        return ESP_FAIL;
    }

    // Get file size
    fseek(read_file, 0, SEEK_END);
    long raw_file_size = ftell(read_file);
    fseek(read_file, 0, SEEK_SET);

    // Read file content
    char *file_content = malloc(raw_file_size + 1);
    if (file_content == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for cleanup");
        fclose(read_file);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_FAIL;
    }

    fread(file_content, 1, raw_file_size, read_file);
    fclose(read_file);
    file_content[raw_file_size] = '\0';

    // Find and remove multipart boundary - robust reverse search algorithm
    size_t clean_file_size = raw_file_size;
    bool boundary_found = false;

    ESP_LOGI(TAG, "Searching for multipart boundary in %ld bytes...", raw_file_size);

    // Debug: Show last 100 bytes before cleanup
    if (raw_file_size > 100)
    {
        ESP_LOGI(TAG, "Last 100 bytes before cleanup:");
        for (long i = raw_file_size - 100; i < raw_file_size; i++)
        {
            if (file_content[i] >= 32 && file_content[i] <= 126)
            {
                printf("%c", file_content[i]);
            }
            else
            {
                printf("[%02X]", (uint8_t)file_content[i]);
            }
        }
        printf("\n");
    }

    // Search backwards from end of file for boundary pattern
    // This is more reliable as boundaries are typically at the end
    if (raw_file_size > 50)
    { // Need minimum size to have boundary

        // Look for the boundary signature starting from the end
        for (long pos = raw_file_size - 10; pos >= 50; pos--)
        {

            // Check for boundary pattern at this position
            if (pos >= 6 && memcmp(&file_content[pos - 6], "------", 6) == 0)
            {

                // Found "------", now check if it's followed by "WebKitFormBoundary"
                if (pos + 18 < raw_file_size &&
                    memcmp(&file_content[pos], "WebKitFormBoundary", 18) == 0)
                {

                    ESP_LOGI(TAG, "Found boundary pattern at position %ld", pos - 6);

                    // Find the start of this line by going backwards
                    long line_start = pos - 6;

                    // Go back to find newline before boundary
                    while (line_start > 0)
                    {
                        char c = file_content[line_start - 1];
                        if (c == '\n' || c == '\r')
                        {
                            break; // Found newline, stop here
                        }
                        line_start--;
                    }

                    clean_file_size = line_start;
                    boundary_found = true;

                    ESP_LOGI(TAG, "Boundary line starts at position %ld", line_start);
                    ESP_LOGI(TAG, "Clean file size after boundary removal: %zu bytes (was %ld)",
                             clean_file_size, raw_file_size);
                    break;
                }
            }
        }
    }

    // If reverse search didn't work, try forward search as fallback
    if (!boundary_found)
    {
        ESP_LOGI(TAG, "Reverse search failed, trying forward search...");

        char *boundary_pos = strstr(file_content, "------WebKitFormBoundary");
        if (boundary_pos != NULL)
        {

            // Find start of line containing this boundary
            char *line_start = boundary_pos;
            while (line_start > file_content &&
                   *(line_start - 1) != '\n' &&
                   *(line_start - 1) != '\r')
            {
                line_start--;
            }

            clean_file_size = line_start - file_content;
            boundary_found = true;

            ESP_LOGI(TAG, "Forward search found boundary at position %ld",
                     (long)(boundary_pos - file_content));
            ESP_LOGI(TAG, "Clean file size: %zu bytes (was %ld)", clean_file_size, raw_file_size);
        }
    }

    if (!boundary_found)
    {
        ESP_LOGW(TAG, "No multipart boundary found, keeping original size: %ld bytes", raw_file_size);
    }
    else
    {
        // Final cleanup: remove any trailing whitespace before boundary
        while (clean_file_size > 0)
        {
            char c = file_content[clean_file_size - 1];
            if (c == '\r' || c == '\n' || c == ' ' || c == '\t')
            {
                clean_file_size--;
            }
            else
            {
                break;
            }
        }
        ESP_LOGI(TAG, "Final clean file size after whitespace removal: %zu bytes", clean_file_size);
    }

    ESP_LOGI(TAG, "Final clean file size: %zu bytes", clean_file_size);

    // Debug: Show last 50 bytes of cleaned content
    if (clean_file_size > 0)
    {
        ESP_LOGI(TAG, "Last 50 bytes of cleaned file:");
        size_t start_pos = (clean_file_size > 50) ? clean_file_size - 50 : 0;
        for (size_t i = start_pos; i < clean_file_size; i++)
        {
            if (file_content[i] >= 32 && file_content[i] <= 126)
            {
                printf("%c", file_content[i]);
            }
            else
            {
                printf("[%02X]", (uint8_t)file_content[i]);
            }
        }
        printf("\n");
    }

    // Write clean content back to file
    FILE *clean_file = fopen(FIRMWARE_FILE_PATH, "wb");
    if (clean_file == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file for writing clean content");
        free(file_content);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "File cleanup failed");
        return ESP_FAIL;
    }

    fwrite(file_content, 1, clean_file_size, clean_file);
    fclose(clean_file);
    free(file_content);

    ESP_LOGI(TAG, "File upload completed successfully. Clean file size: %zu bytes", clean_file_size);

    // Check if uploaded file is HEX format and convert to BIN
    bool is_hex_file = false;

    // Check file extension from Content-Disposition header in the first chunk
    char *filename_start = strstr(header_buffer, "filename=\"");
    if (filename_start != NULL)
    {
        filename_start += 10; // Skip 'filename="'
        char *filename_end = strchr(filename_start, '"');
        if (filename_end != NULL)
        {
            size_t filename_len = filename_end - filename_start;
            if (filename_len > 4)
            {
                char *ext_pos = filename_start + filename_len - 4;
                if (strncmp(ext_pos, ".hex", 4) == 0 || strncmp(ext_pos, ".HEX", 4) == 0)
                {
                    is_hex_file = true;
                    ESP_LOGI(TAG, "Detected HEX file from filename, will convert to BIN");
                }
            }
        }
    }

    // If no filename in header, check file content for HEX format
    if (!is_hex_file)
    {
        FILE *check_file = fopen(FIRMWARE_FILE_PATH, "r");
        if (check_file != NULL)
        {
            char first_line[32];
            if (fgets(first_line, sizeof(first_line), check_file) != NULL)
            {
                if (first_line[0] == ':')
                {
                    is_hex_file = true;
                    ESP_LOGI(TAG, "Detected HEX format by content, will convert to BIN");
                }
            }
            fclose(check_file);
        }
    }

    if (is_hex_file)
    {
        ESP_LOGI(TAG, "Converting HEX file to BIN format...");

        // Rename current file to temp hex file
        if (rename(FIRMWARE_FILE_PATH, TEMP_HEX_FILE_PATH) != 0)
        {
            ESP_LOGE(TAG, "Failed to rename HEX file");
            http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_FAILED);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "HEX file processing failed");
            return ESP_FAIL;
        }

        // Convert HEX to BIN
        if (convert_hex_to_bin(TEMP_HEX_FILE_PATH, FIRMWARE_FILE_PATH) != ESP_OK)
        {
            ESP_LOGE(TAG, "HEX to BIN conversion failed");
            // Restore original file
            rename(TEMP_HEX_FILE_PATH, FIRMWARE_FILE_PATH);
            http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_FAILED);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "HEX to BIN conversion failed");
            return ESP_FAIL;
        }

        // Remove temp hex file
        remove(TEMP_HEX_FILE_PATH);

        // Get new binary file size
        FILE *bin_file = fopen(FIRMWARE_FILE_PATH, "rb");
        if (bin_file != NULL)
        {
            fseek(bin_file, 0, SEEK_END);
            long bin_size = ftell(bin_file);
            fclose(bin_file);
            ESP_LOGI(TAG, "HEX to BIN conversion completed. Binary size: %ld bytes", bin_size);
        }
    }

    ESP_LOGI(TAG, "Firmware upload and processing completed successfully");

    // Send success message
    http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_SUCCESSFULL);

    // Send response
    httpd_resp_set_type(req, "text/plain");
    if (is_hex_file)
    {
        httpd_resp_send(req, "HEX file uploaded and converted to BIN successfully", HTTPD_RESP_USE_STRLEN);
    }
    else
    {
        httpd_resp_send(req, "BIN file uploaded successfully", HTTPD_RESP_USE_STRLEN);
    }

    return ESP_OK;
}

// Handler for firmware download (/download POST)
static esp_err_t http_server_download_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Firmware download to STM32 started with protocol");

    // Check if firmware file exists
    FILE *file = fopen(FIRMWARE_FILE_PATH, "rb");
    if (file == NULL)
    {
        ESP_LOGE(TAG, "Firmware file not found");
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Firmware file not found. Please upload firmware first.");
        return ESP_FAIL;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0)
    {
        ESP_LOGE(TAG, "Invalid firmware file size: %ld", file_size);
        fclose(file);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid firmware file");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Firmware file size: %ld bytes", file_size);

    // Read entire file into memory for checksum calculation
    uint8_t *firmware_data = malloc(file_size);
    if (firmware_data == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for firmware data");
        fclose(file);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_FAIL;
    }

    size_t bytes_read = fread(firmware_data, 1, file_size, file);
    fclose(file);

    if (bytes_read != file_size)
    {
        ESP_LOGE(TAG, "Failed to read complete firmware file");
        free(firmware_data);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "File read error");
        return ESP_FAIL;
    }

    // Calculate checksum
    uint32_t firmware_checksum = calculate_checksum(firmware_data, file_size);
    ESP_LOGI(TAG, "Firmware checksum: 0x%08lX", firmware_checksum);

    // Send OTA update initialized message
    http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_INITIALIZED);

    // Clear UART buffers before starting protocol
    uart_flush(UART_PORT_NUM);

    // Small delay before main protocol
    //vTaskDelay(pdMS_TO_TICKS(100));

    // Step 1: Send FW_REQUEST command (using byte protocol)
    ESP_LOGI(TAG, "Step 1: Sending FW_REQUEST");
    if (send_command_byte(FW_REQUEST) != ESP_OK)
    {
        free(firmware_data);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send FW_REQUEST");
        return ESP_FAIL;
    }

    // // Reduce delay - give STM32 moderate time for reset but not too long
    // vTaskDelay(pdMS_TO_TICKS(300)); // Reduced from 500ms to 300ms

    // Clear any spurious responses from UART buffer
    uart_flush(UART_PORT_NUM);
    vTaskDelay(pdMS_TO_TICKS(20)); // Reduced settling time

    // Step 2: Wait for FW_READY response with retry mechanism
    ESP_LOGI(TAG, "Step 2: Waiting for FW_READY");

    // Try up to 3 times to get FW_READY
    bool fw_ready_received = false;
    for (int retry = 0; retry < 3 && !fw_ready_received; retry++)
    {
        if (retry > 0)
        {
            ESP_LOGW(TAG, "FW_READY retry attempt %d/3", retry + 1);
            // Send FW_REQUEST again
            uart_flush(UART_PORT_NUM);
            //vTaskDelay(pdMS_TO_TICKS(50));
            if (send_command_byte(FW_REQUEST) != ESP_OK)
            {
                continue;
            }
            //vTaskDelay(pdMS_TO_TICKS(200));
        }

        if (wait_for_response_byte(FW_READY, 2000) == ESP_OK)
        { // Reduced timeout to 2s
            fw_ready_received = true;
        }
    }

    if (!fw_ready_received)
    {
        ESP_LOGE(TAG, "STM32 completely unresponsive after 3 attempts - may need hardware reset");
        free(firmware_data);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "STM32 not ready for firmware update");
        return ESP_FAIL;
    }

    // Step 3: Send FW_LENGTH command
    ESP_LOGI(TAG, "Step 3: Sending FW_LENGTH: %ld bytes", file_size);
    if (send_command_with_data(FW_LENGTH, (uint32_t)file_size) != ESP_OK)
    {
        free(firmware_data);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send FW_LENGTH");
        return ESP_FAIL;
    }

    // Step 4: Wait for FW_OK or FW_ERR response
    ESP_LOGI(TAG, "Step 4: Waiting for FW_OK or FW_ERR");
    esp_err_t length_response = wait_for_fw_ok_or_err(PROTOCOL_TIMEOUT_MS);
    if (length_response == ESP_FAIL)
    {
        free(firmware_data);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "STM32 rejected firmware length (FW_ERR)");
        return ESP_FAIL;
    }
    else if (length_response != ESP_OK)
    {
        free(firmware_data);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "STM32 length response timeout");
        return ESP_FAIL;
    }

    // Step 5: Send firmware data in chunks
    ESP_LOGI(TAG, "Step 5: Starting firmware data transmission");
    size_t total_sent = 0;
    size_t offset = 0;

    while (offset < file_size)
    {
        size_t chunk_size = (file_size - offset > DATA_CHUNK_SIZE) ? DATA_CHUNK_SIZE : (file_size - offset);

        // Give STM32 time to prepare for data
        vTaskDelay(pdMS_TO_TICKS(10));

        // Send chunk data
        int bytes_written = uart_write_bytes(UART_PORT_NUM, (const char *)(firmware_data + offset), chunk_size);
        if (bytes_written != chunk_size)
        {
            ESP_LOGE(TAG, "UART write error: expected %zu, sent %d", chunk_size, bytes_written);
            free(firmware_data);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "UART transmission error");
            return ESP_FAIL;
        }

        // Debug: Log what we're sending
        ESP_LOGI(TAG, "Sent %zu bytes of firmware data to STM32", chunk_size);

        // Wait for FW_RECEIVED response (STM32 acknowledges each chunk)
        if (wait_for_response_byte(FW_RECEIVED, 15000) != ESP_OK)
        { // Increase timeout to 15 seconds to give STM32 more time
            ESP_LOGE(TAG, "STM32 did not acknowledge byte at offset %zu", offset);
            free(firmware_data);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "STM32 did not acknowledge data byte");
            return ESP_FAIL;
        }

        // Clear UART buffer after each successful chunk to prevent overflow
        uart_flush(UART_PORT_NUM);

        // Conservative delay between chunks for maximum reliability (1 byte per chunk)
//        vTaskDelay(pdMS_TO_TICKS(200)); // Increased back to 200ms for 1-byte chunks to ensure reliability

        offset += chunk_size;
        total_sent += chunk_size;

        // Log progress less frequently for 1-byte chunks to avoid spam
        if (total_sent % 64 == 0 || total_sent == file_size)
        { // Every 64 bytes for 1-byte chunks
            int progress = (total_sent * 100) / file_size;
            ESP_LOGI(TAG, "Progress: %d%% (%zu/%ld bytes) - Byte %zu completed",
                     progress, total_sent, file_size, total_sent);
        }
    }

    ESP_LOGI(TAG, "Firmware data transmission completed. Total sent: %zu bytes", total_sent);

    // Step 6: Send checksum for verification
    ESP_LOGI(TAG, "Step 6: Sending checksum: %lu (0x%08lX)", firmware_checksum, firmware_checksum);
    if (send_command_with_data(CHECKSUM_DATA, firmware_checksum) != ESP_OK)
    {
        free(firmware_data);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send checksum");
        return ESP_FAIL;
    }

    // Step 7: Wait for checksum verification result
    ESP_LOGI(TAG, "Step 7: Waiting for checksum verification");

    // Wait for either CHECKSUM_OK or CHECKSUM_ERR (using byte protocol)
    TickType_t start_time = xTaskGetTickCount();
    TickType_t timeout_ticks = pdMS_TO_TICKS(PROTOCOL_TIMEOUT_MS);
    bool checksum_result = false;
    bool response_received = false;

    while ((xTaskGetTickCount() - start_time) < timeout_ticks && !response_received)
    {
        uint8_t byte;
        int bytes_read = uart_read_bytes(UART_PORT_NUM, &byte, 1, pdMS_TO_TICKS(100));

        if (bytes_read > 0)
        {
            ESP_LOGI(TAG, "Received byte during checksum verification: %d (0x%02X)", byte, byte);
            
            // Check for CHECKSUM_OK
            if (byte == CHECKSUM_OK)
            {
                ESP_LOGI(TAG, "Checksum verification: SUCCESS");
                checksum_result = true;
                response_received = true;
                break;
            }

            // Check for CHECKSUM_ERR
            if (byte == CHECKSUM_ERR)
            {
                ESP_LOGE(TAG, "Checksum verification: FAILED");
                checksum_result = false;
                response_received = true;
                break;
            }

            ESP_LOGW(TAG, "Received unexpected checksum response: %d (expected: CHECKSUM_OK=%d or CHECKSUM_ERR=%d)", 
                     byte, CHECKSUM_OK, CHECKSUM_ERR);
        }
    }

    // Clean up firmware data
    free(firmware_data);

    if (!response_received)
    {
        ESP_LOGE(TAG, "Timeout waiting for checksum verification");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Checksum verification timeout");
        return ESP_FAIL;
    }

    if (!checksum_result)
    {
        ESP_LOGE(TAG, "Firmware transfer failed - checksum mismatch");
        http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_FAILED);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Firmware transfer failed - checksum error");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Firmware download completed successfully with checksum verification");

    // Send success message
    http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_SUCCESSFULL);

    // Send response
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, "Firmware downloaded to STM32 successfully with checksum verification", HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}
// set up default  httpd server configuration
static httpd_handle_t http_server_configure(void)
{
    // default httpd config
    httpd_config_t http_config = HTTPD_DEFAULT_CONFIG();

    // create http server monitor task
    xTaskCreatePinnedToCore(&http_server_monitor,
                            "http_server_monitor_task",
                            HTTP_SERVER_MONITOR_STACK_SIZE,
                            NULL,
                            HTTP_SERVER_MONITOR_PRIORITY,
                            &task_http_server_monitor,
                            HTTP_SERVER_MONITOR_CORE_ID);
    // create the message queue
    http_server_monitor_queue_handle = xQueueCreate(3, sizeof(http_server_queue_message_t));
    // Chinh lai core id, priority, stack size
    http_config.core_id = HTTP_SERVER_TASK_CORE_ID;
    http_config.task_priority = HTTP_SERVER_TASK_PRIORITY;
    http_config.stack_size = HTTP_SERVER_TASK_STACK_SIZE;
    // tang uri handlers
    http_config.max_uri_handlers = 20;
    // tang timeout limit
    http_config.recv_wait_timeout = 10;
    http_config.send_wait_timeout = 10;

    //(optional) xuat ra log thong bao cau hinh http server
    ESP_LOGI(TAG, "http server config: Port: '%d', task priority: '%d'",
             http_config.server_port, http_config.task_priority);
    // start httpd server
    if (httpd_start(&http_server_handle, &http_config) == ESP_OK) ///////////
    {
        ESP_LOGI(TAG, "http server config: Register URI handlers");
        // register index.html handler
        httpd_uri_t index_html = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = http_server_index_html_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(http_server_handle, &index_html);
        // register script.js
        httpd_uri_t script_js = {
            .uri = "/webpage/script.js",
            .method = HTTP_GET,
            .handler = http_server_script_js_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(http_server_handle, &script_js);
        // register style.css
        httpd_uri_t style_css = {
            .uri = "/webpage/style.css",
            .method = HTTP_GET,
            .handler = http_server_style_css_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(http_server_handle, &style_css);
        // Register upload handler (POST /upload)
        httpd_uri_t upload = {
            .uri = "/upload",
            .method = HTTP_POST,
            .handler = http_server_upload_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(http_server_handle, &upload);
        // Register download handler (POST /download)
        httpd_uri_t download = {
            .uri = "/download",
            .method = HTTP_POST,
            .handler = http_server_download_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(http_server_handle, &download);
        return http_server_handle;
    }
    return NULL;
}

void http_server_start(void)
{
    if (http_server_handle == NULL)
    {
        http_server_handle = http_server_configure();
    }
}

void http_server_stop(void)
{
    if (http_server_handle)
    {
        httpd_stop(http_server_handle);
        ESP_LOGI(TAG, "HTTP server: STOPPING");
        http_server_handle = NULL;
    }
    if (task_http_server_monitor)
    {
        vTaskDelete(task_http_server_monitor);
        ESP_LOGI(TAG, "HTTP server: stopping http server monitor");
        task_http_server_monitor = NULL;
    }
}

BaseType_t http_server_monitor_send_message(http_server_message_e msgID)
{
    http_server_queue_message_t msg;
    msg.msgID = msgID;
    return xQueueSend(http_server_monitor_queue_handle, &msg, portMAX_DELAY);
}