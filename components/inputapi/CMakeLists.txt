idf_component_register(
    SRCS "joystick_input.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_adc esp_driver_gpio esp_timer log
)
