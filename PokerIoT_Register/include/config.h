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

#define RST_PIN 5
#define SS_PIN 15
#define OLED_SDA 21
#define OLED_SCL 22

// Shared variables
extern Adafruit_SH1106 display;
extern WebSocketsClient webSocket;
extern bool cardScanned;
extern String cardId;

// Function declarations
void initDisplay();
void showMessage(const String &line1, const String &line2);
void initRFID();
void connectToWiFi();
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);
void initWebSocket();
bool checkRFIDCard();