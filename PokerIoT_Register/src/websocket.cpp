#include "config.h"

WebSocketsClient webSocket;
extern String cardId;

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
    if (type != WStype_TEXT)
        return;
    Serial.print("[WS] Raw message: ");
    Serial.println((const char *)payload);
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
        return;

    const char *eventType = doc["event"];
    if (!eventType)
    {
        Serial.println("[ERROR] Missing 'event' in JSON payload.");
        serializeJson(doc, Serial); // Print full document
        return;
    }
    if (strcmp(eventType, "register_player_ack") == 0)
    {
        const char *username = doc["username"] | "Unknown";
        showMessage("Registered:", username);
    }
    else if (strcmp(eventType, "error") == 0)
    {
        const char *msg = doc["message"];
        showMessage("Error:", msg);
    }
}

void initWebSocket()
{
    webSocket.begin(host, port, path);
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(5000);
}
