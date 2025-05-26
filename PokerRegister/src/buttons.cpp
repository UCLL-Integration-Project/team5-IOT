#include "config.h"
#include <Arduino.h>

void initButtons()
{
    pinMode(BUTTON_UP, INPUT_PULLUP);
    pinMode(BUTTON_DOWN, INPUT_PULLUP);
    pinMode(BUTTON_CONFIRM, INPUT_PULLUP);
    pinMode(BUTTON_RETURN, INPUT_PULLUP);
}