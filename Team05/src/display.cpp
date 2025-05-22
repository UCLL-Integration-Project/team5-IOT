#include "config.h"

SH1106Wire display(0x3C, OLED_SDA, OLED_SCL);

void initDisplay()
{
    display.init();
    display.flipScreenVertically();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.clear();
    display.drawString(0, 0, "Poker System Ready");
    display.display();
}

void showMessage(const String &line1, const String &line2)
{
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, line1);
    display.drawString(0, 12, line2);
    display.display();
}