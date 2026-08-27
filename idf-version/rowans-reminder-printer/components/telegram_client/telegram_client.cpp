#include "telegram_client.h"

#include <string.h>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "lwip/netdb.h"
#include "esp_http_client.h"
#include "cJSON.h"

TelegramClient::TelegramClient(const char* my_token) 
    : token(my_token) {
    ESP_LOGI("TELEGRAM CLIENT", "Telegram client initialized");
}

esp_err_t TelegramClient::sendMessage(const char* message, const char* chat_id) {
    esp_http_client_config_t config = {};
    config.url = "";
    config.method = HTTP_METHOD_POST;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);

    return ESP_OK;
}

esp_err_t TelegramClient::getMessages() {
    return ESP_OK;
}