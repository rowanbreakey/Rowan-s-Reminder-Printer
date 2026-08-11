#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <map>
#include <chrono>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>
#include "secrets.h"

using namespace std;

const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;

Preferences prefs;

bool reset_disp = true;

String super_user_state = "";
String username_save = "";

#define BOTtoken PRINTER_BOT_TOKEN
#define SUPER_USER_CHAT_ID USER_ID

std::map<String, String> user_dict;

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

const unsigned long BOT_MTBS = 1000; 
unsigned long lastTimeBotRan = 0;

#define I2C_SDA 21
#define I2C_SCL 22

LiquidCrystal_I2C lcd(0x27, 20, 4);

bool isVerified(String uid) {
  if (uid == SUPER_USER_CHAT_ID) {
    return true;
  }

  prefs.begin("users", true);
  String name = prefs.getString(uid.c_str(), "");
  prefs.end();
  return (name.length() > 0);
}

String getName(String uid) {
  if (uid == SUPER_USER_CHAT_ID) {
    return "Rowan";
  }
  prefs.begin("users", true);
  String name = prefs.getString(uid.c_str(), "");
  prefs.end();

  return name;
}

void addUser(String uid, String name) {
  prefs.begin("users", false);
  prefs.putString(uid.c_str(), name);
  prefs.end();
}

void handleNewMessages(int numNewMessages) {
  lcd.clear();
  lcd.print("Processing:");
  lcd.setCursor(0, 1);
  lcd.print(numNewMessages);
  lcd.print(" new message(s)...");
  delay(500);

  for (int i = 0; i < numNewMessages; i++) {
    String sender_chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;

    reset_disp = true;

    if (isVerified(sender_chat_id)) {
      if (sender_chat_id == SUPER_USER_CHAT_ID && text == "/adduser" && super_user_state == "") {
        bot.sendMessage(SUPER_USER_CHAT_ID, "Please enter the name of the new user.");
        super_user_state = "awaiting username";
      } else if (sender_chat_id == SUPER_USER_CHAT_ID && super_user_state == "awaiting username") {
        username_save = text;
        bot.sendMessage(SUPER_USER_CHAT_ID, "Please enter the user id for " + text + ".");
        super_user_state = "awaiting user id";
      } else if (sender_chat_id == SUPER_USER_CHAT_ID && super_user_state == "awaiting user id") {
        addUser(text, username_save);
        bot.sendMessage(SUPER_USER_CHAT_ID, "New user initialized successfully.");
        super_user_state = "";
      } else {
        time_t unix_time = (time_t) bot.messages[i].date.toInt();
        struct tm* timeinfo = localtime(&unix_time);
        char time_buf[64];
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", timeinfo);

        Serial2.println("================================");
        Serial2.println("  ROWAN'S REMINDER PRINTER  ");
        Serial2.println("================================");
        Serial2.println("\n");
        Serial2.print("Sent By: ");
        Serial2.println(getName(sender_chat_id));
        Serial2.println("\n");
        Serial2.print("Sent At: ");
        Serial2.println(time_buf);
        Serial2.print("Message: ");
        Serial2.println(text);
        Serial2.println("\n");
        Serial2.println("--------------------------------");
        Serial2.println("\n\n\n");
        bot.sendMessage(sender_chat_id, "Reminder sent successfully!");
      }
    } else {
      bot.sendMessage(sender_chat_id, "Sorry, you are not permited to send Rowan reminders.");
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  delay(200);
  Serial2.write(0x1B);
  Serial2.write(0x40);
  delay(100);

  Wire.begin(I2C_SDA, I2C_SCL);

  lcd.init();
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(0, 0);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  lcd.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    lcd.print(".");
  }

  lcd.clear();
  lcd.print("WiFi Connected!");
  delay(1000);
  lcd.clear();

  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  lcd.print("Setting Up");
  configTzTime("EST5EDT,M3.2.0,M11.1.0", "pool.ntp.org");
  time_t now = time(nullptr);

  while (now < 24 * 3600) {
    lcd.print(".");
    delay(500);
    now = time(nullptr);
  }

  lcd.clear();
  lcd.print("Done!");
  delay(500);

  bool sent = bot.sendMessage(SUPER_USER_CHAT_ID, "ESP32 initialized successfully!", "");
}

void loop() {
  if (reset_disp) {
    lcd.clear();
    lcd.print("Rowan's Reminder");
    lcd.setCursor(0, 1);
    lcd.print("Printer");
    reset_disp = false;
  }
  if (millis() - lastTimeBotRan > BOT_MTBS) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    lastTimeBotRan = millis();
  }
}