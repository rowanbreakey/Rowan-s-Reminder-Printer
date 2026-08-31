#pragma once

#include <cstdint>
#include "driver/gpio.h"
#include "driver/i2c_master.h"

class LCD {
    public:
        LCD(gpio_num_t sda, gpio_num_t scl, uint8_t address);

        esp_err_t init_lcd();

        esp_err_t clear();

        esp_err_t set_cursor(int row, int col);

        esp_err_t write(const char* text);

        esp_err_t backlight();
    private:
        gpio_num_t sda;
        gpio_num_t scl;
        uint8_t address;
        i2c_master_bus_handle_t i2c_bus;
        i2c_master_dev_handle_t lcd_handle;
        uint8_t backlight_state;

        void write_byte(uint8_t data, uint8_t mode);
};
 