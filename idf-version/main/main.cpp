#include <cstring>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
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

#define TOGGLE_PIN GPIO_NUM_4
int state = 0;
int last_state = 0;

void init_nvs(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
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
        }
    }
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
