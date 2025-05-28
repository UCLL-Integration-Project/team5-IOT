#include "config.h"

void setup()
{
  Serial.begin(115200);

  initDisplay();
  Serial.println("Display init done.");
  initRFID();
  Serial.println("RFID init done.");
  connectToWiFi();
  Serial.println("WiFi connected!");
  initWebSocket();
  showMessage("Register Mode", "Scan cards");
}

void loop()
{
  webSocket.loop();
  if (checkRFIDCard())
  {
    StaticJsonDocument<128> doc;
    doc["event"] = "register_player";
    doc["cardId"] = cardId;
    String jsonStr;
    serializeJson(doc, jsonStr);
    webSocket.sendTXT(jsonStr);
    showMessage("Card Registered", cardId);
    delay(1500);
  }
}

void connectToWiFi()
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Connecting to WIFI...");
  display.display();

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    display.print(".");
    display.display();
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Wifi connected!");
  display.print("IP: ");
  display.print(WiFi.localIP());
  display.display();
  delay(2000);
}