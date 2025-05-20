#include <Adafruit_SH1106.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>
#include <HTTPClient.h>
#include "config.h.txt"

/**
 * Poker Player RFID Terminal
 * - Reads RFID card (MFRC522)
 * - Displays player menu on OLED (SBC OLED01)
 * - Accepts input from 4 buttons: Menu Up, Menu Down, Confirm, Return
 * - Sends selected poker action + UID to backend using WebSocket (socket.io)
 */

/**
 *
 * RFID-RC522
 *
 * Pin layout used:
 * ------------------------------------
 *             MFRC522      ESP32
 * Signal      Pin          Pin
 * ------------------------------------
 * RST/Reset   RST          5
 * SPI SS      SDA(NSS)     15
 * SPI MOSI    MOSI         23
 * SPI MISO    MISO         19
 * SPI SCK     SCK          18
 *
 *             Oled(I2C)                ESP32
 * Signal      Pin                      Pin
 * ---------------------------------------------
 * SCL         SCL(Serial Clock)        22
 * SDA         SDA(Serial Data)         21
 *
 *
 *                  Joystick (5V to 3V)      ESP32
 * Signal           Pin                      Pin
 * -------------------------------------------------
 * VCC              + 5V                     3.3V +
 * GND              GND                      GND -
 * VRx(horizontal)  VRx                      34
 * VRy(vertical)    VRy                      35
 * SW(Switch)       SW                       32
 *
 *
 */

// Function Declartions
void connectToWiFi();
void handleRFIDScan();
void handleConfirmButton();
void handleButtonPresses();
void updateMenuDisplay();
void handleWebSocketEvent(JsonDocument &doc);
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);
void displayGameStartMessage();
void displayErrorMessage(const char *errormsg);
void resetGameState();

// Pin Definitions

#define SS_PIN 15
#define RST_PIN 5

#define BUTTON_UP 2
#define BUTTON_DOWN 4
#define BUTTON_CONFIRM 16
#define BUTTON_RETURN 17

#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_RESET -1
#define i2C_ADDRESS 0x3C

Adafruit_SH1106 display;
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Networking clients
WiFiClient Wificlient;
SocketIOclient socketIO;
WebSocketsClient webSocket;
HTTPClient http;

// Game fucntion
String playerUID = ""; // Variable to store the UID of the player;
String playerName = "Guest";
int playerBalance = 0;
int playersNeeded = 0;
int currentSelection = 0;
const char *menuItems[] = {"Check", "Call", "Bet", "Fold"};
int betValue = 0;
int minBet = 10;
int maxBet = 500;
int potValue = 0;
bool cardScanned = false;
bool isMyTurn = false;
bool isRegisteringPlayers = true;

// Button
unsigned long lastDebounceTime = 0; // millis retrun type
const int debounceDelay = 50;

void setup()
{
  Serial.begin(115200);

  // Display Config
  display.begin(SH1106_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.display();

  // RFID Config
  SPI.begin();        // Initializes the SPI bus by setting SCK, MOSI, and SS to outputs, pulling SCK and MOSI low, and SS high.
  mfrc522.PCD_Init(); // Initialize Scanner kit

  // Buttons Config
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_CONFIRM, INPUT_PULLUP);
  pinMode(BUTTON_RETURN, INPUT_PULLUP);

  // Wifi connection
  connectToWiFi();

  // websocket connection
  webSocket.begin(host, port, path); // Starts the websocket connection
  webSocket.onEvent(webSocketEvent); // Hosts a event?
  webSocket.setReconnectInterval(5000);
}
void loop()
{
  webSocket.loop();

  if (cardScanned && !isRegisteringPlayers)
  {
    handleButtonPresses();
    updateMenuDisplay();
  }
  else
  {
    handleRFIDScan();
  }
  delay(10);
}

// Connect to the wifi (if there is connection)
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

// This function checks your card if present
void handleRFIDScan()
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println("Scan your card");
  display.display();

  // Look for new cards to scan
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
  {
    return;
  }

  // read player uid
  playerUID = "";
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    playerUID += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
    playerUID += String(mfrc522.uid.uidByte[i], HEX);
  }
  playerUID.toUpperCase();

  JsonDocument doc;
  doc["event"] = "register_player";
  doc["uid"] = playerUID;

  if (playerName == "Guest")
  {
    doc["name"] = "Player_" + playerUID.substring(0, 4);
  }
  else
  {
    doc["name"] = playerName;
  }

  String output;
  serializeJson(doc, output);
  webSocket.sendTXT(output);

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Registration sent");
  display.print("UID: ");
  display.println(playerUID);
  display.display();
  delay(1000);
}

void handleButtonPresses()
{
  unsigned long now = millis();
  if (now - lastDebounceTime < debounceDelay)
    return;

  if (digitalRead(BUTTON_UP) == LOW && isMyTurn)
  {
    if (strcmp(menuItems[currentSelection], "Bet") == 0)
    {
      betValue = min(maxBet, betValue + 10);
    }
    else
    {
      currentSelection = (currentSelection - 1 + 4) % 4;
    }
    lastDebounceTime = now;
    updateMenuDisplay();
  }
  else if (digitalRead(BUTTON_DOWN) == LOW && isMyTurn)
  {
    if (strcmp(menuItems[currentSelection], "Bet") == 0)
    {
      betValue = max(minBet, betValue - 10);
    }
    else
    {
      currentSelection = (currentSelection + 1) % 4;
    }
    lastDebounceTime = now;
    updateMenuDisplay();
  }
  else if (digitalRead(BUTTON_CONFIRM) == LOW && isMyTurn)
  {
    handleConfirmButton(); // Sends action to backend
    lastDebounceTime = now;
  }
  else if (digitalRead(BUTTON_RETURN) == LOW)
  {
    if (strcmp(menuItems[currentSelection], "Bet") == 0 && betValue > minBet)
    {
      betValue = max(minBet, betValue - 10);
    }
    else
    {
      cardScanned = false;
      playerUID = "";
    }
    lastDebounceTime = now;
    updateMenuDisplay();
  }
}

void handleConfirmButton()
{
  if (strcmp(menuItems[currentSelection], "Bet") == 0 && betValue < minBet)
  {
    betValue = minBet;
    return;
  }

  JsonDocument doc;
  doc["uid"] = playerUID;
  doc["action"] = menuItems[currentSelection];

  if (strcmp(menuItems[currentSelection], "Bet") == 0)
  {
    doc["amount"] = betValue;
  }

  String output;
  serializeJson(doc, output);
  webSocket.sendTXT(output);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Action sent:");
  display.println(menuItems[currentSelection]);
  if (strcmp(menuItems[currentSelection], "Bet") == 0)
  {
    display.print("Amount: ");
    display.println(betValue);
  }
  display.display();
  delay(1000);

  isMyTurn = false;
  betValue = minBet;
}

void updateMenuDisplay()
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print(playerName);
  display.print(" ");
  display.println(playerBalance);

  display.setTextSize(3);
  display.setCursor(display.width() / 2 - 30, display.height() / 2 - 10);
  display.println(menuItems[currentSelection]);

  display.setTextSize(1);
  display.setCursor(0, display.height() - 10);
  display.print("pot :");
  display.println(potValue);

  if (isMyTurn && !isRegisteringPlayers)
  {
    display.setCursor(0, display.height() - 20);
    display.print("UP/DOWN: Change CONFIRM: Select");
  }
  display.display();
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_CONNECTED:
    Serial.println("Connected to server");
    break;
  case WStype_DISCONNECTED:
    Serial.println("Disconnected from server");
    break;
  case WStype_TEXT:
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
      Serial.println("JSON deserialization failed!");
      return;
    }
    handleWebSocketEvent(doc);
    break;
  }
}

void handleWebSocketEvent(JsonDocument &doc)
{
  const char *event = doc["event"];

  if (strcmp(event, "game_update") == 0)
  {
    potValue = doc["pot"];
    isMyTurn = (doc["current_player"].as<String>() == playerUID);

    for (JsonObject player : doc["players"].as<JsonArray>())
    {
      if (player["uid"] == playerUID)
      {
        playerBalance = player["balance"];
      }
    }
  }
  else if (strcmp(event, "registration_ack") == 0)
  {
    playerName = doc["name"] | "Guest";
    isRegisteringPlayers = doc["is_registering"] | true;
    playersNeeded = doc["players_needed"] | 0;

    Serial.println("Registered as: " + playerName);
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Registered: " + playerName);
    if (isRegisteringPlayers)
    {
      display.println("Waiting for players...");
      display.println("Players needed: " + String(playersNeeded));
    }
    else
    {
      display.println("All players ready!");
    }
    display.display();
  }
  else if (strcmp(event, "game_start") == 0)
  {
    isRegisteringPlayers = false;
    displayGameStartMessage();
  }
  else if (strcmp(event, "error") == 0)
  {
    const char *errorMsg = doc["message"] | "Unknown error";
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("ERROR:");
    display.println(errorMsg);
    display.display();
    delay(2000);
  }
}

void displayGameStartMessage()
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(3);
  display.println("Game Start!");
  display.display();
  delay(1000);
}

void displayErrorMessage(const char *errormsg)
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println("Error:");
  display.print(errormsg);
  display.display();
  delay(1500);
}

void resetGameState()
{
  cardScanned = false;
  isMyTurn = false;
  isRegisteringPlayers = true;
  playerName = "Guest";
  betValue = 0;
  currentSelection = 0;
  menuItems[0] = "Check";
  menuItems[1] = "Bet";
  menuItems[2] = "Call";
  menuItems[3] = "Raise";
  updateMenuDisplay();
}