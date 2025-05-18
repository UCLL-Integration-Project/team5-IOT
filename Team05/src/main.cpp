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

// Function Declartions (prototypes)
String getPlayerName(String uid);
void connectToWiFi();
void handleRFIDScan();
void handleMenuNavigation();
void handleConfirmButton();
void updateMenuDisplay();
void socketIOEvent(socketIOmessageType_t type, uint8_t *payload, size_t length);
void handleSocketIOEvent(uint8_t *payload, size_t length);

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
int currentSelection = 0;
const char *menuItems[] = {"Check", "Call", "Bet", "Fold"};
int betValue = 0;
int minBet = 10;
int maxBet = 500;
int potValue = 0;
int potValue = 0;
bool cardScanned = false;
bool isMyTurn = false;

// Button
unsigned long lastDebouceTime = 0; // millis retrun type
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

  if (!cardScanned)
  {
    handleRFIDScan();
  }
  else
  {
    handleMenuNavigation();
  }
  delay(10); // This ensures your button press is only accepted once every 50 milliseconds, smoothing out the bounces.
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

  sendPlayerData("player_scan", playerUID);
  cardScanned = true;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Authenticating...");
  display.display(); // show the content
  // delay(1000); // Delay before displaying/clearing uid
}

void handleMenuNavigation()
{
  static unsigned long lastButtonPress = 0;

  // This ensures your button press is only accepted once every 200 milliseconds, smoothing out the bounces.
  if ((millis() - lastButtonPress) < lastDebouceTime)
    return;

  // current seletion is 0, our menu has 4 choices so to circle through the menu we have to insure that the value sits 0 - 3 3 as in last menu item.
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
    lastDebouceTime = millis();
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
    lastDebouceTime = millis();
  }
  else if (digitalRead(BUTTON_CONFIRM) == LOW && isMyTurn)
  {
    handleConfirmButton();
    lastDebouceTime = millis();
  }
  else if (digitalRead(BUTTON_RETURN) == LOW)
  {
    if (betValue > minBet)
    {
      betValue = max(minBet, betValue - 10);
    }
    else
    {
      cardScanned = false;
      playerUID = "";
    }
    lastDebouceTime = millis();
  }
  updateMenuDisplay(); // relects the new state of the screen
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
  display.setCursor(0, 0);

  display.print(playerName);
  display.print(" ");
  display.println(playerBalance);

  display.print("pot :");
  display.println(potValue);
  display.println("-------------");

  for (int i = 0; i < 4; i++)
  {
    if (i == currentSelection && isMyTurn)
    {
      display.print("> ");
    }
    else
    {
      display.print(" ");
    }

    display.print(menuItems[i]);
    if (strcmp(menuItems[i], "Bet") == 0 && i == currentSelection)
    {
      display.print(": ");
      display.print(betValue);
    }
    display.println();
  }
  display.println("-------------");
  if (isMyTurn)
  {
    display.println("YOUR TURN");
  }
  else
  {
    display.println("Waiting...");
  }

  display.display();
}

void sendPlayerData(const char *event, String uid)
{
  JsonDocument doc;

  doc["event"] = event;
  doc["uid"] = event;

  String output;
  serializeJson(doc, output);
  webSocket.sendTXT(output);
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

    String event = doc["event"];

    if (event == "player_data")
    {
      playerName = doc["name"].as<String>();
      playerBalance = doc["balance"];
      potValue = doc["pot"];
      isMyTurn = doc["your_turn"];

      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Welcome " + playerName);
      display.print("Balance: ");
      display.println(playerBalance);
      display.display();
      delay(1000);
    }
    else if (event == "game_update")
    {
      playerBalance = doc["balance"];
      potValue = doc["pot"];
      isMyTurn = doc["your_turn"];

      String actionMessage = doc["message"].as<String>();
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println(actionMessage);
      display.display();
      delay(1000);
    }
    break;
  }
}
