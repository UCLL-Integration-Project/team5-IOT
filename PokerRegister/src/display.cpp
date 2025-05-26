#include "config.h"

Adafruit_SH1106 display;

void initDisplay()
{
    display.begin(SH1106_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Poker System Ready");
    display.display();
}

void showMessage(const String &line1, const String &line2)
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(line1);
    display.println(line2);
    display.display();
}