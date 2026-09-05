#include "printer_driver.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"

Printer::Printer(gpio_num_t tx, gpio_num_t rx, int baud_rate)
    : TX(tx), RX(rx), BAUD_RATE(baud_rate), PORT_NUM(UART_NUM_1), BUF_SIZE(1024) {
        uart_config_t uart_config = {};
        uart_config.baud_rate = 9600;
        uart_config.data_bits = UART_DATA_8_BITS;
        uart_config.parity    = UART_PARITY_DISABLE;
        uart_config.stop_bits = UART_STOP_BITS_1;
        uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
        uart_config.source_clk = UART_SCLK_DEFAULT;

        uart_param_config(PORT_NUM, &uart_config);
        uart_set_pin(PORT_NUM, TX, RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        uart_driver_install(PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);

        const char *init_cmd = "\x1B\x40";
        uart_write_bytes(PORT_NUM, init_cmd, 2);
    }

esp_err_t Printer::print_line(const char* text) {
    uart_write_bytes(PORT_NUM, text, strlen(text));
    uart_write_bytes(PORT_NUM, "\n", 1);
    return ESP_OK;
}