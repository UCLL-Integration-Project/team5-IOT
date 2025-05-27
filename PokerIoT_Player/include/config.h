#pragma once // prevents duplicate initialization
#include <Adafruit_SH1106.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <HTTPClient.h>
#include <stdint.h>

extern const char *ssid;
extern const char *password;
extern const char *host;
extern const char *path;
extern const uint16_t port;

// Pins definition
#define BUTTON_UP 2
#define BUTTON_DOWN 4
#define BUTTON_CONFIRM 16
#define BUTTON_RETURN 17

#define RST_PIN 5
#define SS_PIN 15
#define OLED_SDA 21
#define OLED_SCL 22

#define LOCAL_DEV true

// Shared variables
extern Adafruit_SH1106 display;
extern WebSocketsClient webSocket;

extern const char *menuItems[5];
extern int currentSelection;
extern bool inAmountMenu;
extern int betValue;
extern const int minBet;
extern const int betStep;
extern const int minRaise;

extern bool actionTaken;
extern int lastBet;
extern int playerBalance;
extern bool cardScanned;
extern String cardId;

// Function declarations
void initButtons();
void initDisplay();
void showMessage(const String &line1, const String &line2);
void initRFID();
bool checkRFIDCard();
void connectToWiFi();
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);
void initWebSocket();
void sendGameUpdate(const char *action, int amount = 0);
void handleButtonPresses();
void updateMenuDisplay();