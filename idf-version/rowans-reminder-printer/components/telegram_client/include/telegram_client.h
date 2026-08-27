#pragma once

#include "esp_err.h"

class TelegramClient {
    public:
        TelegramClient(const char* my_token);

        esp_err_t sendMessage(const char* message, const char* chat_id);

        esp_err_t getMessages();

    private:
        const char* token;
};