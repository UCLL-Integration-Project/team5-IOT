#include "config.h"

WebSocketsClient webSocket;
extern String cardId;
extern bool cardScanned;
bool isRegisteringPlayers = true;
extern int lastBet;
extern int playerBalance;

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
    if (type != WStype_TEXT)
        return;
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
        return;

    const char *eventType = doc["event"];

    if (strcmp(eventType, "register_player_ack") == 0)
    {
        const char *username = doc["username"];
        showMessage("Registered:", username);
    }
    else if (strcmp(eventType, "registration_ack") == 0)
    {
        const char *name = doc["name"] | "Player";
        isRegisteringPlayers = doc["is_registering"] | true;
        showMessage("Registered:", name);
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
        for (JsonObject player : doc["players"].as<JsonArray>())
        {
            if (player["cardId"] == cardId)
            {
                playerBalance = player["balance"];
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
    StaticJsonDocument<256> doc;
    doc["event"] = "game_update";
    doc["action"] = action;
    doc["amount"] = amount;
    doc["cardId"] = cardId;
    String jsonStr;
    serializeJson(doc, jsonStr);
    webSocket.sendTXT(jsonStr);
}
