#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "link_layer.h"
#include "transport_layer.h"
#include "joystick_input.h"

#define TAG "Remote main"
SemaphoreHandle_t wifi_sem = NULL;
SemaphoreHandle_t arm_seq_sem = NULL;

void app_main(void)
{
    wifi_sem = xSemaphoreCreateBinary() ;
    arm_seq_sem = xSemaphoreCreateBinary() ;

    wifiConnect("DZ", "DRONE LINK", "drone1234");

    while(wifiStatus != 1010){
        printf("WiFi Status: %d\n", wifiStatus);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    
    if(wifi_sem != NULL){
        ESP_LOGI(TAG,"Waiting for Wi-Fi connection with the Drone.");
        xSemaphoreTake(wifi_sem, portMAX_DELAY);
        ESP_LOGI(TAG,"Wi-Fi connection succes !") ;
    }


    xTaskCreate(transport_task, "transport_task", 4096, NULL, 5, NULL);
    
    if(arm_seq_sem != NULL){
        ESP_LOGI(TAG, "Waiting for Hello Remote.") ;
        xSemaphoreTake(arm_seq_sem, portMAX_DELAY);
        ESP_LOGI(TAG, "Hello Remote received !") ;
    }


    for(;;){
        vTaskDelay(pdTICKS_TO_MS(10));
    }
}
