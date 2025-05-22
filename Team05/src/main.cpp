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
  Serial.println("WebSocket init done.");
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
  display.clear();
  display.drawString(0, 0, "Connecting to WiFi...");
  display.display();

  WiFi.begin(ssid, password);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries++ < 20)
  {
    delay(500);
    display.drawString(0, 12, ".");
    display.display();
  }

  display.clear();
  if (WiFi.status() == WL_CONNECTED)
  {
    display.drawString(0, 0, "WiFi connected");
    display.drawString(0, 12, WiFi.localIP().toString());
  }
  else
  {
    display.drawString(0, 0, "WiFi failed.");
  }
  display.clear();
  display.display();
  delay(1500);
}