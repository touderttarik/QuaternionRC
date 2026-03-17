idf_component_register(
    SRCS "matrix.c" "quaternions.c" "vectors.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_i2c esp_driver_mcpwm
)
