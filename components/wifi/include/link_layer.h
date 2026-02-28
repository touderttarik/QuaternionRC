#pragma once

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include <string.h>

extern volatile int wifiStatus;

void wifiConnect(const char* country, const char* ssid, const char* password);
