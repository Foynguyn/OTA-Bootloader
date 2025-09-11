#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "lwip/netdb.h"

#include "wifi_app.h"
#include "tasks_common.h"
#include "http_server.h"
//#include <string.h>
//tag used for esp serial console messages
static const char TAG [] = "wifi_app";

//queue handle to manipulate events
static QueueHandle_t wifi_app_queue_handle;

esp_netif_t* esp_netif_sta = NULL;
esp_netif_t* esp_netif_ap = NULL;

//5
static void wifi_app_event_handler(void *arg, esp_event_base_t event_base, 
                                    int32_t event_id,  void* event_data)
{
    if(event_base == WIFI_EVENT)
    {
        switch(event_id)
        {
            case WIFI_EVENT_AP_START:
                ESP_LOGI(TAG,"WIFI_EVENT_AP_START");
                break;
            case WIFI_EVENT_AP_STOP:
                ESP_LOGI(TAG,"WIFI_EVENT_AP_STOP");
                break;
            case WIFI_EVENT_AP_STACONNECTED:
                ESP_LOGI(TAG,"WIFI_EVENT_AP_STACONNECTED");
                break;
            case WIFI_EVENT_AP_STADISCONNECTED:
                ESP_LOGI(TAG,"WIFI_EVENT_AP_STADISCONNECTED");
                break;
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG,"WIFI_EVENT_STA_START");
                break;
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG,"WIFI_EVENT_STA_CONNECTED");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG,"WIFI_EVENT_STA_DISCONNECTED");
                break;
        }
    }
    else if(event_base == IP_EVENT)
    {
        switch(event_id)
        {
            case IP_EVENT_STA_GOT_IP:
                ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");
                break;
        }
    }
}
//4
static void wifi_app_event_handler_init(void)
{
    //event loop for wifi driver
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    //event handler for connection
    esp_event_handler_instance_t instance_wifi_event;
    esp_event_handler_instance_t instance_ip_event;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_app_event_handler,
                                                        NULL,
                                                        &instance_wifi_event));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_app_event_handler,
                                                        NULL,
                                                        &instance_ip_event));
}
//6
static void wifi_app_default_wifi_init()
{
    //initialize tcp stack
    ESP_ERROR_CHECK(esp_netif_init());
    //Default wifi config  - sap xep theo thu tu
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    esp_netif_sta = esp_netif_create_default_wifi_sta();
    esp_netif_ap = esp_netif_create_default_wifi_ap();
}
//7
static void wifi_app_softap_config()
{
    wifi_config_t ap_config = 
    {
        .ap = 
        {
            .ssid = WIFI_AP_SSID,
            .ssid_len = strlen(WIFI_AP_SSID),
            .password = WIFI_AP_PASSWORD,
            .channel = WIFI_AP_CHANNEL,
            .ssid_hidden = WIFI_AP_SSID_HIDDEN,
            .max_connection = WIFI_AP_MAX_CONNECTIONS,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .beacon_interval = WIFI_AP_BEACON_INTERVAL,
        },
    };
    //configure dhcp 
    esp_netif_ip_info_t ap_ip_info;
    memset(&ap_ip_info, 0x00, sizeof(ap_ip_info));

    esp_netif_dhcps_stop(esp_netif_ap); //phai goi truoc 
    inet_pton(AF_INET, WIFI_AP_IP, &ap_ip_info.ip); //assign ap's static ip, gateway, netmask
    inet_pton(AF_INET, WIFI_AP_GATEWAY, &ap_ip_info.gw);
    inet_pton(AF_INET, WIFI_AP_NETMASK, &ap_ip_info.netmask);

    ESP_ERROR_CHECK(esp_netif_set_ip_info(esp_netif_ap, &ap_ip_info)); //cau hinh tinh giao dien mang
    ESP_ERROR_CHECK(esp_netif_dhcps_start(esp_netif_ap)); // bat dau dhcp server (phat ra wifi)

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_AP_BANDWIDTH)); //20 MHz/////////////////////
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_STA_POWER_SAVE)); //none
}
//3
void wifi_app_task(void *pvParameters)
{
    wifi_app_queue_message_t msg;

    //initialize the event handler
    wifi_app_event_handler_init();
    //initialize the tcp/ip stack and wifi config
    wifi_app_default_wifi_init();
    //soft AP config
    wifi_app_softap_config();
    //start wifi
    ESP_ERROR_CHECK(esp_wifi_start());
    //send first message
    wifi_app_send_message(WIFI_APP_MSG_START_HTTP_SERVER);
    while(1)
    {
        if(xQueueReceive(wifi_app_queue_handle, &msg, portMAX_DELAY))
        {
            switch(msg.msgID)
            {
                case WIFI_APP_MSG_START_HTTP_SERVER:
                    ESP_LOGI(TAG, "WIFI_APP_MSG_START_HTTP_SERVER");
                    http_server_start();
                    break;
                case WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER:
                    ESP_LOGI(TAG, "WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER");
                    break;
                case WIFI_APP_MSG_STA_CONNECTED_GOT_IP:
                    ESP_LOGI(TAG, "WIFI_APP_MSG_STA_CONNECTED_GOT_IP");
                    break;
                default: 
                    break;
            }
        }
    }
}
//2
BaseType_t wifi_app_send_message(message_app_message_e msgID)
{
    wifi_app_queue_message_t msg;
    msg.msgID = msgID;  
    return xQueueSend(wifi_app_queue_handle, &msg, portMAX_DELAY);
}

//1
void wifi_app_start(void)
{
    ESP_LOGI(TAG, "Starting WiFi application...");
    esp_log_level_set("wifi", ESP_LOG_NONE);
    wifi_app_queue_handle = xQueueCreate(3, sizeof(wifi_app_queue_message_t));
    /*
    BaseType_t xTaskCreatePinnedToCore(TaskFunction_t pvTaskCode, 
                                        const char *constpcName, 
                                        const uint32_t usStackDepth, 
                                        void *constpvParameters, 
                                        UBaseType_t uxPriority, 
                                        TaskHandle_t *constpvCreatedTask, 
                                        const BaseType_t xCoreID)
    */
    xTaskCreatePinnedToCore(&wifi_app_task, 
                            "wifi_app_task", 
                            WIFI_APP_TASK_STACK_SIZE,
                            NULL, 
                            WIFI_APP_TASK_PRIORITY, 
                            NULL, 
                            WIFI_APP_TASK_CORE_ID);
}