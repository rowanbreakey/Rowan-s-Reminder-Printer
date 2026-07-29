void setup() {
  Serial.begin(115200); // USB Monitor
  Serial2.begin(9600, SERIAL_8N1, 16, 17); // Hardware Serial 2
}

void loop() {
  Serial2.println("PING");
  if (Serial2.available()) {
    String response = Serial2.readStringUntil('\n');
    Serial.print("MAX3232 Echoed back: ");
    Serial.println(response);
  }
  delay(1000);
}