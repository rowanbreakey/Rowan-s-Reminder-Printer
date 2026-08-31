#include "lcd_driver.h"
#include <cstdint>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LCD_BACKLIGHT 0x08
#define LCD_EN        0x04
#define LCD_RS        0x01

LCD::LCD(gpio_num_t sda, gpio_num_t scl, uint8_t address)
    : sda(sda), scl(scl), address(address), i2c_bus(nullptr), lcd_handle(nullptr) {
        i2c_master_bus_config_t bus_config = {};
        bus_config.i2c_port = I2C_NUM_0;
        bus_config.sda_io_num = sda;
        bus_config.scl_io_num = scl;
        bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
        bus_config.glitch_ignore_cnt = 7;
        bus_config.flags.enable_internal_pullup = true;
        ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));

        i2c_device_config_t dev_config = {};
        dev_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
        dev_config.device_address = address;
        dev_config.scl_speed_hz = 100000;
        ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus, &dev_config, &lcd_handle));
}

void LCD::write_byte(uint8_t data, uint8_t mode) {
    uint8_t high_nibble = data & 0xF0;
    uint8_t low_nibble = (data << 4) & 0xF0;

    uint8_t bytes[4];
    bytes[0] = high_nibble | mode | LCD_BACKLIGHT | LCD_EN;
    bytes[1] = high_nibble | mode | LCD_BACKLIGHT;
    bytes[2] = low_nibble  | mode | LCD_BACKLIGHT | LCD_EN;
    bytes[3] = low_nibble  | mode | LCD_BACKLIGHT;

    i2c_master_transmit(lcd_handle, bytes, 4, pdMS_TO_TICKS(100));
}

esp_err_t LCD::init_lcd() {
    vTaskDelay(pdMS_TO_TICKS(50)); // Wait for power-up

    // Force 4-bit initialization sequence per HD44780 datasheet
    write_byte(0x03, 0);
    vTaskDelay(pdMS_TO_TICKS(5));
    write_byte(0x03, 0);
    vTaskDelay(pdMS_TO_TICKS(1));
    write_byte(0x03, 0);
    write_byte(0x02, 0); // Set to 4-bit mode

    // Functional configuration commands
    write_byte(0x28, 0); // 2 lines, 5x8 matrix
    write_byte(0x0C, 0); // Display ON, Cursor OFF
    write_byte(0x06, 0); // Increment cursor
    write_byte(0x01, 0); // Clear display
    vTaskDelay(pdMS_TO_TICKS(5));

    return ESP_OK;
}

esp_err_t LCD::clear() {
    return ESP_OK;
}

esp_err_t LCD::set_cursor(int row, int col) {
    return ESP_OK;
}

esp_err_t LCD::write(const char* text) {
    return ESP_OK;
}

esp_err_t LCD::backlight() {
    return ESP_OK;
}