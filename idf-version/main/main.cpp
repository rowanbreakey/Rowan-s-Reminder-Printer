#include <cstring>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "secrets.h"
#include "telegram_client.h"
#include "lcd_driver.h"
#include "printer_driver.h"

/*
plan:
1.wifi init and evnet handler
3.telegram component
4.I2C LCD library
5.UART lib for printer if necessary
6.task for seeing if new message
7.queue for storing new messages
8.task to print new message / fulfill command
*/

static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

TelegramClient telegram(PRINTER_BOT_TOKEN);
QueueHandle_t telegramQueue = xQueueCreate(10, sizeof(TelegramMessage));
long offset = 0;

gpio_num_t sda = GPIO_NUM_21;
gpio_num_t scl = GPIO_NUM_22;
LCD* lcd = nullptr;

gpio_num_t tx = GPIO_NUM_17;
gpio_num_t rx = GPIO_NUM_16;
Printer* printer = nullptr;

nvs_handle_t allowed_users;

#define TOGGLE_PIN GPIO_NUM_4
int state = 0;
int last_state = 0;

std::string super_user_state = "";
std::string username_save = "";

void init_nvs(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_err_t err = nvs_open("storage", NVS_READWRITE, &allowed_users);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Done Opening NVS handle\n");
    }
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void init_wifi(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {}; 

    std::strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid), SECRET_WIFI_SSID, sizeof(wifi_config.sta.ssid));

    std::strncpy(reinterpret_cast<char*>(wifi_config.sta.password), SECRET_WIFI_PASSWORD, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void get_new_messages_task(void *pvParameters) {
    while (1) {
        telegram.getMessages(telegramQueue, offset);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void handle_queued_messages_task(void *pvParameters) {
    TelegramMessage incomingMsg;

    while (1) {
        if (xQueueReceive(telegramQueue, &incomingMsg, portMAX_DELAY) == pdPASS) {
            puts(incomingMsg.text);
            printer->print_line(incomingMsg.text);
            printer->print_line("\n\n");
            vTaskDelay(pdMS_TO_TICKS(200));
            //when this actually does different things based on the message it must send back that i am not taking messages if state is 0 (off)
            std::string sender_chat_id_std_string = std::to_string(incomingMsg.senderId);
            const char* sender_chat_id = sender_chat_id_std_string.c_str();
            const char* text = incomingMsg.text;

            if (is_verified(sender_chat_id)) {
                if (sender_chat_id == SUPER_USER_ID && text == "/addUser" && super_user_state == "") {
                    telegram.sendMessage("Please enter the name of the new user.", SUPER_USER_ID);
                    super_user_state = "add - awaiting username";
                } else if (sender_chat_id == SUPER_USER_ID && super_user_state == "add - awaiting username") {
                    username_save = text;
                    telegram.sendMessage((std::string("Please enter the user id for ") + text + ".").c_str(), SUPER_USER_ID);
                    super_user_state = "add - awaiting user id";
                } else if (sender_chat_id == SUPER_USER_ID && super_user_state == "add - awaiting user id") {
                    add_user(text, username_save.c_str());
                    telegram.sendMessage("New user initialized successfully.", SUPER_USER_ID);
                    super_user_state = "";
                } else if (sender_chat_id == SUPER_USER_ID && text == "/removeUser" && super_user_state == "") {
                    telegram.sendMessage("Please enter the user id of the user you would like to remove.", SUPER_USER_ID);
                    super_user_state = "remove - awaiting user id";
                } else if (sender_chat_id == SUPER_USER_ID && super_user_state == "remove - awaiting user id") {
                    bool was_removed = remove_user(text);
                    if (was_removed) {
                    telegram.sendMessage(SUPER_USER_ID, "User successfully removed.");
                    } else {
                    telegram.sendMessage(SUPER_USER_ID, "The user id provided was not tied to a verified user.");
                    }
                    super_user_state = "";
                } else {
                    if (state) {
                        std::string time_str = std::to_string(incomingMsg.timeStamp);
                        const char* time = time_str.c_str();
                        printer->print_line("================================");
                        printer->print_line("  ROWAN'S REMINDER PRINTER  ");
                        printer->print_line("================================");
                        printer->print_line("Sent By: ");
                        printer->print_line(get_name(sender_chat_id));
                        printer->print_line("Sent At: ");
                        printer->print_line(time);
                        printer->print_line("\n");
                        printer->print_line("Message: ");
                        printer->print_line(incomingMsg.text);
                        printer->print_line("--------------------------------");
                        printer->print_line("\n\n");
                        telegram.sendMessage("Reminder sent successfully!", sender_chat_id);
                    } else {
                        telegram.sendMessage("Sorry! Rowan isn't accepting reminders right now.", sender_chat_id);
                    }
                }
            } else {
                telegram.sendMessage("Sorry, you are not permited to send Rowan reminders.", sender_chat_id);
            }
        }
    }
}

bool is_verified(const char* uid) {

}

void add_user(const char* username, const char* uid) {

}

bool remove_user(const char* uid) {

}

const char* get_name(const char* uid) {

}

void print_default() {
    lcd->clear();
    vTaskDelay(pdMS_TO_TICKS(10));
    lcd->set_cursor(0, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    lcd->write("Rowan's");
    vTaskDelay(pdMS_TO_TICKS(10));
    lcd->set_cursor(1, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    lcd->write("Reminder");
    vTaskDelay(pdMS_TO_TICKS(10));
    lcd->set_cursor(2, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    lcd->write("Printer");
    vTaskDelay(pdMS_TO_TICKS(10));
}

void init_switch() {
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << TOGGLE_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE; 
    
    gpio_config(&io_conf);
    state = gpio_get_level(TOGGLE_PIN);
    last_state = !state;
}

void check_toggle_state_task(void* pvParameters) {
    while (1) {
        state = gpio_get_level(TOGGLE_PIN);

        if (state != last_state && state == 0) {
            lcd->clear();
            vTaskDelay(pdMS_TO_TICKS(10));
            lcd->backlight_on();
            vTaskDelay(pdMS_TO_TICKS(10));
            print_default();
        } else if (state != last_state && state == 1) {
            lcd->clear();
            vTaskDelay(pdMS_TO_TICKS(10));
            lcd->backlight_off();
        }

        last_state = state;
    }
}

extern "C" void app_main(void) {
    printer = new Printer(tx, rx, 9600);

    lcd = new LCD(sda, scl, 0x27);
    lcd->init_lcd();
    vTaskDelay(pdMS_TO_TICKS(10));
    lcd->clear();
    vTaskDelay(pdMS_TO_TICKS(10));
    lcd->set_cursor(0, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    lcd->backlight_on();
    vTaskDelay(pdMS_TO_TICKS(10));
    lcd->write("Connecting to Wifi...");
    vTaskDelay(pdMS_TO_TICKS(10));

    init_nvs();

    init_wifi();

    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

    ESP_LOGI("MAIN", "WIFI CONNECTED");
    lcd->clear();
    vTaskDelay(pdMS_TO_TICKS(10));
    lcd->set_cursor(0, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    lcd->write("Connected!");
    vTaskDelay(pdMS_TO_TICKS(10));
    telegram.sendMessage("ESP32 Connected and Ready to Receive Messages.", SUPER_USER_ID);

    init_switch();

    xTaskCreate(get_new_messages_task, "TelegramMessageGetterTask", 4096, NULL, 2, NULL);
    xTaskCreate(handle_queued_messages_task, "MessageHandlerTask", 4096, NULL, 1, NULL);
    xTaskCreate(check_toggle_state_task, "ToggleSwitchStateTask", 4096, NULL, 3, NULL);
}
