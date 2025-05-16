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

struct Player
{
  const char *uid;
  const char *name;
};

Player players[] = {
    {"DB F7 1A E3", "Rijensh"}};

const int playerCount = sizeof(players) / sizeof(Player);

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

// Clients
WiFiClient Wificlient;
SocketIOclient socketIO;
HTTPClient http;

// Game fucntion
String playerUID = ""; // Variable to store the UID of the player;
String playerName = "";
int currentSelection = 0;
const char *menuItems[] = {"Check", "Call", "Bet", "Fold"};
int betValue = 0;
bool cardScanned = false;
int pot = 0;

void setup()
{
  Serial.begin(115200);

  // Display Config
  display.begin(SH1106_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Aurora");
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

  // SocketIO
  socketIO.begin(host, port);      // Starts the websocket connection
  socketIO.onEvent(socketIOEvent); // Hosts a event?
}
void loop()
{
  socketIO.loop();

  if (!cardScanned)
  {
    handleRFIDScan();
  }
  else
  {
    handleMenuNavigation();
  }

  delay(50); // This ensures your button press is only accepted once every 50 milliseconds, smoothing out the bounces.
}

String getPlayerName(String uid)
{
  for (int i = 0; i < playerCount; i++)
  {
    if (uid.equalsIgnoreCase(players[i].uid))
    {
      return players[i].name;
    }
  }
  return "Guest";
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

  playerUID = "";
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    playerUID += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
    playerUID += String(mfrc522.uid.uidByte[i], HEX);
  }

  playerUID.toUpperCase();
  cardScanned = true;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Player: ");
  display.print(playerUID);
  display.println("Welcome! ");
  display.display(); // show the content
  delay(1000);       // Delay before displaying/clearing uid
}

void handleMenuNavigation()
{
  static unsigned long lastButtonPress = 0;

  // This ensures your button press is only accepted once every 200 milliseconds, smoothing out the bounces.
  if (millis() - lastButtonPress < 200)
  {
    return;
  }

  // current seletion is 0, our menu has 4 choices so to circle through the menu we have to insure that the value sits 0 - 3 3 as in last menu item.
  if (digitalRead(BUTTON_UP) == LOW)
  {
    currentSelection = (currentSelection - 1 + 4) % 4;
    lastButtonPress = millis();
    Serial.println("UP pressed");
  }

  if (digitalRead(BUTTON_DOWN) == LOW)
  {
    currentSelection = (currentSelection + 1) % 4;
    lastButtonPress = millis();
    Serial.println("DOWN pressed");
  }

  if (digitalRead(BUTTON_CONFIRM) == LOW)
  {
    handleConfirmButton();
    lastButtonPress = millis();
    Serial.println("CONFIRM pressed");
  }

  // pressing return cancels the bet and resets the value to 0.
  // else player wants to back out, UID is cleared, and card scan is removed.
  if (digitalRead(BUTTON_RETURN) == LOW)
  {
    if (betValue > 0)
    {
      betValue = 0;
    }
    else
    {
      cardScanned = false;
      playerUID = "";
    }
    lastButtonPress = millis(); // Stores the current time to help with debouncing (avoiding multiple accidental presses).
    Serial.println("RETURN pressed");
  }
  updateMenuDisplay(); // relects the new state of the screen
}

// If current selection is bet then each press increases the value by 10 till 100 and resets back to 10
// void handleConfirmButton()
// {
//   if (strcmp(menuItems[currentSelection], "Bet") == 0)
//   {
//     betValue += 10;
//     if (betValue > 100)
//     {
//       betValue = 10;
//     }
//     return;
//   }
// }

void handleConfirmButton()
{
  if (strcmp(menuItems[currentSelection], "Bet") == 0 && betValue == 0)
  {
    betValue = 10;
    return;
  }

  Serial.print(playerName);
  Serial.print(" (");
  Serial.print(playerUID);
  Serial.print(") selected: ");
  Serial.print(menuItems[currentSelection]);
  if (strcmp(menuItems[currentSelection], "Bet") == 0)
  {
    Serial.print(" (Amount: ");
    Serial.print(betValue);
    Serial.print(")");
  }
  Serial.println();

  // Show confirmation on display
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Action confirmed:");
  display.print(playerName);
  display.print(" - ");
  display.println(menuItems[currentSelection]);
  if (strcmp(menuItems[currentSelection], "Bet") == 0)
  {
    display.print("Amount: ");
    display.println(betValue);
  }
  display.display();
  delay(1000);

  betValue = 0;
}

void updateMenuDisplay()
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(playerName);
  display.println("------------");

  for (int i = 0; i < 4; i++)
  {
    if (i == currentSelection)
    {
      display.print("> ");
    }
    else
    {
      display.print(" ");
    }

    display.print(menuItems[i]);
    if (strcmp(menuItems[i], "Bet") == 0 && betValue > 0)
    {
      display.print(": ");
      display.print(betValue);
    }
    display.println();
  }
  display.display();
}

void socketIOEvent(socketIOmessageType_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case sIOtype_DISCONNECT:
    Serial.println("[IOc] Disconnected");
    break;
  case sIOtype_CONNECT:
    Serial.println("[Ioc] Connected");
    socketIO.send(sIOtype_CONNECT, "/");
    break;
  case sIOtype_EVENT:
    handleSocketIOEvent(payload, length);
    break;
  case sIOtype_ACK:
  case sIOtype_ERROR:
  case sIOtype_BINARY_ACK:
  case sIOtype_BINARY_EVENT:
    break;
  }
}

void handleSocketIOEvent(uint8_t *payload, size_t length)
{
  char *sptr = NULL;
  int id = strtol((char *)payload, &sptr, 10);

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, sptr, length - (sptr - (char *)payload));

  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  String eventName = doc[0];

  if (eventName == "game_update")
  {
    // Handle game state updates from server
    JsonObject data = doc[1];
    // Process game update here
  }
}
