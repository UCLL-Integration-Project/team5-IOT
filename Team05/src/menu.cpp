#include "config.h"

const char *menuItems[] = {"Check", "Bet", "Call", "Raise", "Fold"};
int currentSelection = 0;
bool inAmountMenu = false;
int betValue = 0;
const int minBet = 10;
const int betStep = 10;
const int minRaise = 10;
int lastBet = 0;
int playerBalance = 9999;

void handleButtonPresses()
{
    if (digitalRead(BUTTON_UP) == LOW)
    {
        if (inAmountMenu)
            betValue += betStep;
        else
            currentSelection = (currentSelection - 1 + 5) % 5;
        delay(200);
    }
    if (digitalRead(BUTTON_DOWN) == LOW)
    {
        if (inAmountMenu && betValue > minBet)
            betValue -= betStep;
        else
            currentSelection = (currentSelection + 1) % 5;
        delay(200);
    }
    if (digitalRead(BUTTON_CONFIRM) == LOW && !actionTaken)
    {
        const char *selected = menuItems[currentSelection];

        if (inAmountMenu)
        {
            if (betValue > playerBalance)
            {
                showMessage("Insufficient Chips", "Lower your bet");
                return;
            }
            if (strcmp(selected, "Raise") == 0 && (betValue <= lastBet || (betValue - lastBet) < minRaise))
            {
                showMessage("Invalid Raise", "Must raise +10");
                return;
            }
            if (strcmp(selected, "Call") == 0 && betValue != lastBet)
            {
                showMessage("Invalid Call", "Must match last bet");
                return;
            }
            sendGameUpdate(selected, betValue);
            inAmountMenu = false;
            actionTaken = true;
        }
        else if (strcmp(selected, "Check") == 0 || strcmp(selected, "Fold") == 0)
        {
            sendGameUpdate(selected, 0);
            actionTaken = true;
        }
        else
        {
            inAmountMenu = true;
            betValue = lastBet > minBet ? lastBet : minBet;
        }
        delay(200);
    }
    if (digitalRead(BUTTON_RETURN) == LOW && inAmountMenu)
    {
        inAmountMenu = false;
        delay(200);
    }
}

void updateMenuDisplay()
{
    display.clear();

    if (actionTaken)
    {
        display.drawString(0, 0, "Action sent.");
        display.drawString(0, 12, "Wait for next round...");
    }
    else if (inAmountMenu)
    {
        display.drawString(0, 0, menuItems[currentSelection]);
        display.drawString(0, 12, "Amount: " + String(betValue));
        display.drawString(0, 24, "Balance: " + String(playerBalance));
    }
    else
    {
        for (int i = 0; i < 5; i++)
        {
            String line = (i == currentSelection ? "> " : "  ");
            line += menuItems[i];
            display.drawString(0, i * 12, line); // 12px line spacing
        }
    }

    display.display();
}
