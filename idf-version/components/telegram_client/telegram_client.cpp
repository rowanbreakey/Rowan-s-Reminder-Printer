#include "telegram_client.h"

#include <string.h>
#include <string>
#include <format>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "lwip/netdb.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "esp_crt_bundle.h"

TelegramClient::TelegramClient(const char* my_token) 
    : token(my_token) {
    ESP_LOGI("TELEGRAM CLIENT", "Telegram client initialized");
}

esp_err_t TelegramClient::sendMessage(const char* message, const char* chat_id) {

    std::string url = std::format("https://api.telegram.org/bot{}/sendMessage", this->token);
    esp_http_client_config_t config = {};
    config.url = url.c_str();
    config.method = HTTP_METHOD_POST;
    config.crt_bundle_attach = esp_crt_bundle_attach;

    char body[256];
    snprintf(body, sizeof(body), "{\"chat_id\": \"%s\", \"text\": \"%s\"}", chat_id, message);

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, body, strlen(body));
    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_cleanup(client);

    return ESP_OK;
}

esp_err_t TelegramClient::getMessages() {
    esp_http_client_config_t config = {};
    config.url = "https://api.telegram.org/bot123456:ABC-DEF1234ghIkl-zyx57W2v1u123ew11/getUpdates";
    config.method = HTTP_METHOD_GET;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
    return ESP_OK;
}