#pragma once

#include "driver/gpio.h"
#include "driver/uart.h"

class Printer {
    private:
        const gpio_num_t TX;
        const gpio_num_t RX;
        const int BAUD_RATE;
        const uart_port_t PORT_NUM;
        const int BUF_SIZE;

    public: 
        Printer(gpio_num_t tx, gpio_num_t rx, int baud_rate);

        esp_err_t print_line(const char* text);
};