#include "config.h"

enum ActionState
{
  WAITING_FOR_CARD,
  SELECTING_ACTION,
  CONFIRMING_ACTION
};

ActionState actionState = WAITING_FOR_CARD;
String scannedCardForTurn = "";
bool playerAddedToGame = false;
String selectedAction = "";
int selectedAmount = 0;
bool actionPending = false;

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
  showMessage("Ready", "Scan card");
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

      // Send game_add_player event once
      if (!playerAddedToGame)
      {
        StaticJsonDocument<128> doc;
        doc["event"] = "game_add_player";
        doc["cardId"] = cardId;
        String jsonStr;
        serializeJson(doc, jsonStr);
        webSocket.sendTXT(jsonStr);
        playerAddedToGame = true;
        showMessage("Added to game", cardId);
      }
      else
      {
        showMessage("Card scanned", "Choose action");
      }

      actionState = SELECTING_ACTION;
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
        if (actionPending)
        {
          sendGameUpdate(selectedAction.c_str(), selectedAmount);
          actionPending = false;
        }
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