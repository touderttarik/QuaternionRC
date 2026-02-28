#include "link_layer.h"

static volatile int retry_num = 0;
volatile int wifiStatus = 1000;

static void wifi_event_handler(void* event_handler_arg, esp_event_base_t event_base,
int32_t event_id, void* event_data){
    
    switch(event_id){
        case WIFI_EVENT_STA_START :
            wifiStatus = 1001 ;
            break ;
        case WIFI_EVENT_STA_CONNECTED :
            wifiStatus = 1002 ;
            break ;
        case WIFI_EVENT_STA_DISCONNECTED : 
            if(retry_num < 5){
                esp_wifi_connect();
                retry_num ++ ;
                wifiStatus = 1001 ; 
            }
            break ;
        case IP_EVENT_STA_GOT_IP:
            wifiStatus = 1010 ;
            break ;
    }

}

void wifiConnect(const char* country, const char* ssid, const char* password){
    (void)country;
    nvs_flash_init() ;
    esp_netif_init() ;
    esp_event_loop_create_default() ;
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL) ;
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL) ;

    //We setup the esp-wifi configuration and associate it with the esp-netif
    wifi_init_config_t wificonfig = WIFI_INIT_CONFIG_DEFAULT() ;
    esp_wifi_init(&wificonfig) ;
    esp_netif_create_default_wifi_sta() ;
    //If we don't create the wifi-netif we can connect to the wifi but we won't have an IP address
    wifi_config_t staconf = {
        .sta = {
            .ssid = "DRONE LINK",
            .password = "drone1234",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strcpy((char*)staconf.sta.ssid, ssid) ;
    strcpy((char*)staconf.sta.password, password) ;
    esp_wifi_set_mode(WIFI_MODE_STA) ;
    esp_wifi_set_config(WIFI_IF_STA, &staconf) ;
    esp_wifi_start() ;
    esp_wifi_connect() ;
}
