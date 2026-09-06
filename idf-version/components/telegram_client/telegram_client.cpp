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

static esp_err_t update_event_handler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (evt->user_data) {
                ((std::string*)evt->user_data)->append((char*)evt->data, evt->data_len);
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

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

    return err;
}

esp_err_t TelegramClient::getMessages(QueueHandle_t messageQueue, long &offset) {
    std::string url = std::format("https://api.telegram.org/bot{}/getUpdates?offset={}&timeout=5", this->token, offset);
    std::string response_data = "";

    esp_http_client_config_t config = {};
    config.url = url.c_str();
    config.method = HTTP_METHOD_GET;
    config.crt_bundle_attach = esp_crt_bundle_attach;
    config.event_handler = update_event_handler;
    config.user_data = &response_data;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK || response_data.empty()) {
        return ESP_FAIL;
    }

    cJSON *root = cJSON_Parse(response_data.c_str());
    if (!root) {
        return ESP_FAIL;
    }

    puts(response_data.c_str());

    cJSON *ok = cJSON_GetObjectItem(root, "ok");
    if (!cJSON_IsTrue(ok)) {
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    cJSON *result = cJSON_GetObjectItem(root, "result");
    int arraySize = cJSON_GetArraySize(result);
    
    if (arraySize == 0) {
        cJSON_Delete(root);
        return ESP_ERR_NOT_FOUND;
    }

    for (int i = 0; i < arraySize; i++) {
        cJSON *update = cJSON_GetArrayItem(result, i);
        cJSON *updateIdObj = cJSON_GetObjectItem(update, "update_id");
        cJSON *messageObj = cJSON_GetObjectItem(update, "message");

        if (messageObj) {
            TelegramMessage outMessage = {};
            cJSON *textObj = cJSON_GetObjectItem(messageObj, "text");
            cJSON *chatObj = cJSON_GetObjectItem(messageObj, "chat");
            cJSON *dateObj = cJSON_GetObjectItem(messageObj, "date");

            if (textObj && textObj->valuestring) {
                strncpy(outMessage.text, textObj->valuestring, sizeof(outMessage.text) - 1);
                outMessage.text[sizeof(outMessage.text) - 1] = '\0';            }
            if (chatObj) {
                cJSON *idObj = cJSON_GetObjectItem(chatObj, "id");
                if (idObj) {
                    outMessage.senderId = (int64_t)idObj->valuedouble;
                }
            }
            if (dateObj) {
                outMessage.timeStamp = (long)dateObj->valuedouble;
            }
            if (updateIdObj) {
                outMessage.updateId = (long)updateIdObj->valuedouble;
                offset = outMessage.updateId + 1; // Advance offset past highest ID processed
            }

            xQueueSend(messageQueue, &outMessage, 0);
        }
    }

    return ESP_OK;

}