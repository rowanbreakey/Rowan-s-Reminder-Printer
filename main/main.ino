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

    Serial.print("From ID: ");
    Serial.println(sender_chat_id);
    Serial.print("Text: ");
    Serial.println(text);

    // Security Check: Make sure only YOU can trigger the bot
    if (sender_chat_id != CHAT_ID) {
      Serial.println("Rejected unauthorized user!");
      bot.sendMessage(sender_chat_id, "Unauthorized user.", "");
      continue;
    }

    // Command Handlers
    if (text == "/start") {
      bot.sendMessage(CHAT_ID, "Printer Bot is online and ready!", "");
    } 
    else if (text == "/ping") {
      bot.sendMessage(CHAT_ID, "Pong!", "");
    } 
    else {
      bot.sendMessage(CHAT_ID, "You said: " + text, "");
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n--- Starting ESP32 Telegram Test ---");

  // 1. Connect to Wi-Fi
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

  // 2. Attach Telegram Certificate
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  // 3. Sync System Time via NTP (Required for HTTPS validation)
  Serial.print("Syncing Time");
  configTime(0, 0, "pool.ntp.org");
  time_t now = time(nullptr);
  while (now < 24 * 3600) {
    Serial.print(".");
    delay(500);
    now = time(nullptr);
  }
  Serial.println("\nTime Synced!");

  // 4. Send Initial Boot Notification
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