#include "config.h"

WebSocketsClient webSocket;
extern String cardId;
extern bool cardScanned;
extern bool actionTaken;
extern int lastBet;
extern int playerBalance;

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
    if (type == WStype_CONNECTED)
    {
        Serial.println("[WS] Connected to backend.");
    }
    else if (type == WStype_DISCONNECTED)
    {
        Serial.println("[WS] Disconnected from backend.");
    }
    else if (type != WStype_TEXT)
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
    else if (strcmp(eventType, "game_add_player_ack") == 0)
    {
        int pos = doc["position"];
        showMessage("Joined Game", String("Position: ") + pos);
    }
    else if (strcmp(eventType, "game_start") == 0)
    {
        actionTaken = false;
        cardScanned = true;
        showMessage("Game started", "Your turn if first");
    }
    else if (strcmp(eventType, "game_update_ack") == 0)
    {
        int pos = doc["position"];
        lastBet = doc["last_bet"];
        showMessage("Update ACK", String("Pos:") + pos + " Bet:" + lastBet);
    }
    else if (strcmp(eventType, "game_update") == 0)
    {
        if (!doc["players"].is<JsonArray>())
            return;
        for (JsonObject player : doc["players"].as<JsonArray>())
        {
            if (player["cardId"] == cardId)
            {
                playerBalance = player["balance"] | playerBalance;
            }
        }
    }
    else if (strcmp(eventType, "game_end") == 0)
    {
        const char *winner = doc["winner"];
        int pot = doc["pot"];
        showMessage("Winner:", String(winner) + " Pot: " + pot);
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

void sendGameUpdate(const char *action, int amount)
{
    if (action == nullptr)
    {
        Serial.println("[ERROR] Action is null!");
        return;
    }

    if (cardId.length() == 0)
    {
        Serial.println("[ERROR] cardId is EMPTY!");
        return;
    }

    Serial.printf("[sendGameUpdate] action: %s, amount: %d, cardId: %s\n",
                  action, amount, cardId.c_str());

    StaticJsonDocument<256> doc;
    doc["event"] = "game_update";
    doc["action"] = action;
    doc["amount"] = amount;
    doc["cardId"] = cardId.c_str();

    String jsonStr;
    serializeJson(doc, jsonStr);
    Serial.println("Sending JSON:");
    Serial.println(jsonStr);

    webSocket.sendTXT(jsonStr);
}
