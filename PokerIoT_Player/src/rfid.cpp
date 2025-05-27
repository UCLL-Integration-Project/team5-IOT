#include "config.h"

MFRC522 rfid(SS_PIN, RST_PIN);
bool cardScanned = false;
String cardId = "";

void initRFID()
{
    SPI.begin();
    rfid.PCD_Init();
}

bool checkRFIDCard()
{
    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
        return false;

    cardId = "";
    for (byte i = 0; i < rfid.uid.size; i++)
    {
        cardId += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
        cardId += String(rfid.uid.uidByte[i], HEX);
    }
    cardId.toUpperCase();
    return true;
}

void handleRFIDScan()
{
    if (checkRFIDCard())
        return;

    StaticJsonDocument<128> doc;
    doc["event"] = isRegisteringPlayers ? "register_player" : "game_add_player";
    doc["cardId"] = cardId;
    String jsonStr;
    serializeJson(doc, jsonStr);
    webSocket.sendTXT(jsonStr);

    cardScanned = true;
    delay(1000);
}
