#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "link_layer.h"
#include "transport_layer.h"

#define TAG "Remote main"
SemaphoreHandle_t sem = NULL;
SemaphoreHandle_t arm_seq_sem = NULL;

void app_main(void)
{
    sem = xSemaphoreCreateBinary() ;
    arm_seq_sem = xSemaphoreCreateBinary() ;
    
    wifiConnect("DZ", "DRONE LINK", "drone1234");

    while(wifiStatus != 1010){
        printf("WiFi Status: %d\n", wifiStatus);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    xTaskCreate(transport_task, "transport_task", 4096, NULL, 5, NULL);
    
    if(sem != NULL){
        ESP_LOGI(TAG, "Waiting for Hello Remote.") ;
        xSemaphoreTake(sem, portMAX_DELAY);
        ESP_LOGI(TAG, "Hello Remote received !") ;
    }
    
    

}
