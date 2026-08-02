#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include "secrets.h"

// Load credentials from secrets.h
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;

#define BOTtoken PRINTER_BOT_TOKEN
#define CHAT_ID USER_ID

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Check for new messages every 1 second
const unsigned long BOT_MTBS = 1000; 
unsigned long lastTimeBotRan = 0;

void handleNewMessages(int numNewMessages) {
  Serial.print("Processing ");
  Serial.print(numNewMessages);
  Serial.println(" message(s)...");

  for (int i = 0; i < numNewMessages; i++) {
    String sender_chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;

    Serial2.println("================================");
    Serial2.println("  ROWAN'S REMINDER PRINTER  ");
    Serial2.println("================================");
    Serial2.print("Sent By: ");
    Serial2.println(sender_chat_id);
    Serial2.print("Message: ");
    Serial2.println(text);
    Serial2.println("--------------------------------");
    Serial2.println("\n\n\n");
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

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  Serial.print("Syncing Time");
  configTime(0, 0, "pool.ntp.org");
  time_t now = time(nullptr);

  while (now < 24 * 3600) {
    Serial.print(".");
    delay(500);
    now = time(nullptr);
  }

  Serial.println("\nTime Synced!");

  Serial.println("Sending boot message to Telegram...");
  bool sent = bot.sendMessage(CHAT_ID, "ESP32 initialized successfully!", "");

  if (sent) {
    Serial.println(">>> SUCCESS: Boot message delivered to your Telegram! <<<");
  } else {
    Serial.println(">>> ERROR: Delivery failed. Re-verify USER_ID string in secrets.h <<<");
  }
}

void loop() {
  if (millis() - lastTimeBotRan > BOT_MTBS) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    lastTimeBotRan = millis();
  }
}