#include "config.h"

const char *menuItems[] = {"check", "bet", "call", "raise", "fold"};
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
            if (strcmp(selected, "raise") == 0 && (betValue <= lastBet || (betValue - lastBet) < minRaise))
            {
                showMessage("Invalid Raise", "Must raise +10");
                return;
            }
            if (strcmp(selected, "call") == 0 && betValue != lastBet)
            {
                showMessage("Invalid Call", "Must match last bet");
                return;
            }
            sendGameUpdate(selected, betValue);
            inAmountMenu = false;
            actionTaken = true;
        }
        else if (strcmp(selected, "check") == 0 || strcmp(selected, "fold") == 0)
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
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);

    if (actionTaken)
    {
        display.setCursor(0, 0);
        display.println("Action sent.");
        display.setCursor(0, 12);
        display.println("Wait for next round...");
    }
    else if (inAmountMenu)
    {
        display.setCursor(0, 0);
        display.println(menuItems[currentSelection]);
        display.setCursor(0, 12);
        display.print("Amount: ");
        display.println(betValue);
        display.setCursor(0, 24);
        display.print("Balance: ");
        display.println(playerBalance);
    }
    else
    {
        for (int i = 0; i < 5; i++)
        {
            display.setCursor(0, i * 10);
            if (i == currentSelection)
                display.print("> ");
            else
                display.print("  ");
            display.println(menuItems[i]);
        }
    }

    display.display();
}
