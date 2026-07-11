#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define I2C_SDA 21
#define I2C_SCL 22

LiquidCrystal_I2C lcd(0x27, 20, 4);

int i = 0;

void setup() {

  Wire.begin(I2C_SDA, I2C_SCL);

  lcd.init();
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("RRP");
}

void loop() {
}