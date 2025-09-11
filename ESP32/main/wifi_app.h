/**
 * WiFi application header file
 */

#ifndef WIFI_APP_H
#define WIFI_APP_H

#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "http_server.h"

//Wifi application settings
#define WIFI_AP_SSID "ESP32"
#define WIFI_AP_PASSWORD "password"
#define WIFI_AP_CHANNEL 1
#define WIFI_AP_SSID_HIDDEN 0
#define WIFI_AP_MAX_CONNECTIONS 5
#define WIFI_AP_BEACON_INTERVAL 100
#define WIFI_AP_IP "192.168.0.1"
#define WIFI_AP_GATEWAY "192.168.0.1"
#define WIFI_AP_NETMASK "255.255.255.0"
#define WIFI_AP_BANDWIDTH WIFI_BW_HT20
#define MAX_CONNECTIONS_RETRIES 5
#define MAX_SSID_LENGTH 32
#define MAX_PASSWORD_LENGTH 64
#define WIFI_STA_POWER_SAVE WIFI_PS_NONE

//netif object for AP/STA
extern esp_netif_t* esp_netif_sta;
extern esp_netif_t* esp_netif_ap;

typedef enum message_app_message
{
    WIFI_APP_MSG_START_HTTP_SERVER = 0,
    WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER,
    WIFI_APP_MSG_STA_CONNECTED_GOT_IP,
} message_app_message_e;

typedef struct wifi_app_queue_message
{
    message_app_message_e msgID;
} wifi_app_queue_message_t;

BaseType_t wifi_app_send_message(message_app_message_e msgID);

/**
 * @brief Start the WiFi application
 */
void wifi_app_start(void);

#endif // WIFI_APP_H