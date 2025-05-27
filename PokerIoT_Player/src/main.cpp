#include "config.h"

enum ActionState
{
  WAITING_FOR_CARD,
  SELECTING_ACTION,
  CONFIRMING_ACTION
};

ActionState actionState = WAITING_FOR_CARD;
String scannedCardForTurn = "";

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
  switch (actionState)
  {
  case WAITING_FOR_CARD:
    if (checkRFIDCard())
    {
      scannedCardForTurn = cardId;
      cardScanned = true;
      actionTaken = false;
      actionState = SELECTING_ACTION;
      showMessage("Card scanned", "Choose action");
    }
    break;

  case SELECTING_ACTION:
    if (!actionTaken)
    {
      handleButtonPresses();
      updateMenuDisplay();
      if (actionTaken)
      {
        showMessage("Confirm Action", "Scan again");
        actionState = CONFIRMING_ACTION;
      }
    }
    break;

  case CONFIRMING_ACTION:
    if (checkRFIDCard())
    {
      if (cardId == scannedCardForTurn)
      {
        showMessage("Action Confirmed", "Waiting...");
        cardScanned = false;
        actionTaken = false;
        actionState = WAITING_FOR_CARD;
      }
      else
      {
        showMessage("Wrong card", "Scan again");
      }
    }
    break;
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