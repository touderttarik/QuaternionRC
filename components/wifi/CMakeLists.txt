idf_component_register(
    SRCS "transport_tools.c" "link_layer.c" "transport_layer.c" "transport_math.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi esp_event nvs_flash esp_netif lwip freertos log inputapi orientationapi commontypes
)
