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
SemaphoreHandle_t sem = NULL;
SemaphoreHandle_t arm_seq_sem = NULL;

static void joystick_monitor_task(void *pvParameters)
{
    (void)pvParameters;

    joystick_values_t v = {0};
    while (1) {
        esp_err_t err = joystick_input_read(&v);
        if (err == ESP_OK) {
            ESP_LOGI(TAG,
                     "JOY clean: roll=%+.3f pitch=%+.3f yaw=%+.3f throttle_stick=%+.3f throttle=%.3f swR=%d swL=%d",
                     v.roll,
                     v.pitch,
                     v.yaw,
                     v.throttle_stick,
                     v.throttle,
                     v.right_switch_pressed,
                     v.left_switch_pressed);
        } else {
            ESP_LOGE(TAG, "joystick_input_read failed: %s", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(50)); // 20 Hz debug print
    }
}

void app_main(void)
{
    sem = xSemaphoreCreateBinary() ;
    arm_seq_sem = xSemaphoreCreateBinary() ;

    wifiConnect("DZ", "DRONE LINK", "drone1234");

    while(wifiStatus != 1010){
        printf("WiFi Status: %d\n", wifiStatus);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    esp_err_t joy_err = joystick_input_init();
    
    if (joy_err != ESP_OK) {
        ESP_LOGE(TAG, "joystick_input_init failed: %s", esp_err_to_name(joy_err));
    } else {
        xTaskCreate(joystick_monitor_task, "joystick_monitor_task", 4096, NULL, 4, NULL);
    }

    xTaskCreate(transport_task, "transport_task", 4096, NULL, 5, NULL);
    
    if(sem != NULL){
        ESP_LOGI(TAG, "Waiting for Hello Remote.") ;
        xSemaphoreTake(sem, portMAX_DELAY);
        ESP_LOGI(TAG, "Hello Remote received !") ;
    }


    //
}
