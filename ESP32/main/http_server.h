/**
 * http server application header file
 */
#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "freertos/FreeRTOS.h"
//#include "portmacro.h"
typedef enum http_server_message
{
    HTTP_MSG_WIFI_CONNECT_INIT = 0,
    HTTP_MSG_WIFI_CONNECT_SUCCESS,
    HTTP_MSG_WIFI_CONNECT_FAIL,
    HTTP_MSG_OTA_UPDATE_SUCCESSFULL,
    HTTP_MSG_OTA_UPDATE_FAILED ,
    HTTP_MSG_OTA_UPDATE_INITIALIZED,
    HTTP_MSG_OTA_PROGRESS,
} http_server_message_e;

typedef struct http_server_queue_message
{
    http_server_message_e msgID;
} http_server_queue_message_t;

BaseType_t http_server_monitor_send_message(http_server_message_e msgID);

void init_uart(void);

esp_err_t init_spiffs(void);

esp_err_t convert_hex_to_bin(const char* hex_file_path, const char* bin_file_path);

void http_server_start(void);

void http_server_stop(void);
#endif