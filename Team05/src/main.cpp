#include "config.h"

void setup()
{
  Serial.begin(115200);
  initButtons();
  Serial.println("Buttons init done");
  initDisplay();
  Serial.println("Display init done.");
  initRFID();
  Serial.println("RFID init done.");
  connectToWiFi();
  Serial.println("WiFi connected!");
  initWebSocket();
  Serial.println("Websockets initialized");
}

void loop()
{
  webSocket.loop();
  if (cardScanned && !isRegisteringPlayers && !actionTaken)
  {
    handleButtonPresses();
    updateMenuDisplay();
  }
  else if (!cardScanned)
  {
    handleRFIDScan();
  }
  delay(50);
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
