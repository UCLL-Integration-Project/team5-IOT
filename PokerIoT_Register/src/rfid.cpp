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

    // Properly end communication with the card
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    return true;
}
