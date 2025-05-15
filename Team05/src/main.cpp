#include <Adafruit_SH1106.h>
#include <Arduino.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>
#include <HTTPClient.h>
#include "config.h"

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

char messageBuffer[256];

// Game fucntion
String playerUID = ""; // Variable to store the UID of the player;
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
  pinMode(BUTTON_DOWN, INPUT_PULLDOWN);
  pinMode(BUTTON_CONFIRM, INPUT_PULLUP);
  pinMode(BUTTON_RETURN, INPUT_PULLUP);

  // Wifi connection
  // connectToWifi();

  // SocketIO
  // socketIO.begin(serverHost, serverPort); // Starts the websocket connection
  // socketIO.onEvent(socketIOEvent);        // Hosts a event?

  void loop()
  {
    socketIO.loop();

    if (!cardScanned)
    {
      handleRIFDScan();
    }
    else
    {
      handleMenuNavigation();
    }
    delay(50); // This ensures your button press is only accepted once every 50 milliseconds, smoothing out the bounces.
  }

  // Connect to the wifi (if there is connection)
  void connectToWifi()
  {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Connecting to WIFI...");
    display.display();

    Wifi.begin(ssid, password);

    while (Wifi.status() != WL_CONNECTED)
    {
      delay(500);
      display.print(".");
      display.display();
    }

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Wifi connected!");
    display.print("IP: ");
    display.printl, (WiFi.localIP());
    display.display();
    delay(2000);
  }

  // This function checks your card if present
  void handleRIFDScan()
  {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Scan your card");
    display.display();

    // Look for new cards to scan
    if (!mfrc522.PICC_IsNewCardPresent())
    {
      return;
    }

    // Select one of the cards
    if (!mfrc522.PICC_ReadCardSerial())
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
    }

    if (digitalRead(BUTTON_DOWN) == LOW)
    {
      currentSelection = (currentSelection + 1) % 4;
      lastButtonPress = millis();
    }

    if (digitalRead(BUTTON_CONFIRM) == LOW)
    {
      handleConfirmButton();
      lastButtonPress = millis();
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
    }
    updateMenuDisplay(); // relects the new state of the screen
  }

  // If current selection is bet then each press increases the value by 10 till 100 and resets back to 10
  void handleConfirmButton()
  {
    if (strcmp(menuItems[currentSelection], "Bet") == 0)
    {
      betValue += 10;
      if (betValue > 100)
      {
        betValue = 10;
      }
      return;
    }
  }

  void updateMenuDisplay()
  {
  }
}