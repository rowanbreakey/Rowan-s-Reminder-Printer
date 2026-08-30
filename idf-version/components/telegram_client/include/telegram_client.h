#pragma once

#include "freertos/FreeRTOS.h"
#include <string>
#include "esp_err.h"
#include "freertos/queue.h"

struct TelegramMessage {
    char text[256];
    long senderId;
    long timeStamp;
    long updateId;
};

class TelegramClient {
    public:
        TelegramClient(const char* my_token);

        esp_err_t sendMessage(const char* message, const char* chat_id);

        esp_err_t getMessages(QueueHandle_t messageQueue, long &offset);

    private:
        const char* token;
};