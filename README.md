# Poker IoT System - ESP32

## Hardware Components

- ESP32 Dev Board (Wi-Fi supported)
- RC522 RFID Reader
- RFID Tags or Cards
- SH1106 OLED Display (128x64 via I2C)
- 4 Push Buttons (UP, DOWN, CONFIRM, RETURN)
- DFPlayer Mini (with speaker)
- 3 LEDs (Red, Green, Yellow)

## Features

- Players use RFID cards to register and join a poker game.
- Game actions (Check, Bet, Raise, Call, Fold) are selected via menu on OLED using buttons.
- WebSocket communication syncs player input with backend.
- DFPlayer Mini plays action-specific audio.
- LEDs indicate game actions with colors.

## Pin Connections

### RFID-RC522 (SPI)

| RFID Pin | ESP32 Pin |
| -------- | --------- |
| RST      | GPIO 5    |
| SDA (SS) | GPIO 15   |
| MOSI     | GPIO 23   |
| MISO     | GPIO 19   |
| SCK      | GPIO 18   |

### OLED Display (I2C)

| OLED Pin | ESP32 Pin |
| -------- | --------- |
| SDA      | GPIO 21   |
| SCL      | GPIO 22   |

### Buttons

| Function | GPIO Pin |
| -------- | -------- |
| UP       | GPIO 2   |
| DOWN     | GPIO 4   |
| CONFIRM  | GPIO 16  |
| RETURN   | GPIO 17  |

### LEDs

| Color  | GPIO Pin |
| ------ | -------- |
| RED    | GPIO 12  |
| GREEN  | GPIO 13  |
| YELLOW | GPIO 14  |

### DFPlayer Mini

| DFPlayer | ESP32 Pin |
| -------- | --------- |
| TX       | GPIO 27   |
| RX       | GPIO 26   |

## Wi-Fi + WebSocket Setup

Add this in `config.h`:

```cpp
const char *ssid = "ucll-integratieproject";
const char *password = "";
const char *host = "team5-backend-itip-project5.apps.okd.ucll.cloud";
const char *path = "/";
const uint16_t port = 3000;
```

## Game Flow

1. Admin registers on web frontend.
2. Players scan RFID to register.
3. When game starts:

   - Each player scans RFID to join.
   - Player navigates menu to choose actions.

4. Actions sent via WebSocket as JSON:

```json
{
  "event": "game_update",
  "action": "Bet",
  "amount": 100,
  "cardId": "04AABBCCDD"
}
```

5. Server replies with `game_update_ack` or `game_end`.

## 🎨 LED Feedback

- RED: Fold
- YELLOW: Check
- GREEN: Bet, Raise, Call
- Blink All: Winner celebration

## 🔊 DFPlayer Audio Index

| Action | Audio Track |
| ------ | ----------- |
| Check  | 1           |
| Bet    | 2           |
| Call   | 3           |
| Raise  | 4           |
| Fold   | 5           |
| Win    | 6           |

---

All components are controlled through modular C++ files using PlatformIO.
