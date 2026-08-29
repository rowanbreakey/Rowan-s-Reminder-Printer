idf_component_register(
    SRCS "telegram_client.cpp"
    INCLUDE_DIRS "include"
    REQUIRES 
        esp_http_client 
        esp-tls
        esp_event
        esp_netif
        nvs_flash
        lwip
        log
        cjson
)