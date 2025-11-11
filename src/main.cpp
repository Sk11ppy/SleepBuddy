#include <ItemBack.h>
#include <ItemLabel.h>
#include <LcdMenu.h>
#include <MenuScreen.h>
#include <RTC.h>
#include <display/LiquidCrystal_I2CAdapter.h>
#include <input/KeyboardAdapter.h>
#include <renderer/CharacterDisplayRenderer.h>
char timeBuffer[9];
char dateBuffer[11];

void updateDateTime() {
    RTCTime now;
    RTC.getTime(now);

    snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d:%02d", now.getHour(),
             now.getMinutes(), now.getSeconds());
    snprintf(dateBuffer, sizeof(dateBuffer), "%02d/%02d/%04d",
             Month2int(now.getMonth()), now.getDayOfMonth(), now.getYear());
}

MENU_SCREEN(mainScreen, mainItems, ITEM_LABEL(timeBuffer),
            ITEM_LABEL(dateBuffer));

LiquidCrystal_I2C lcd(0x20, 16, 2);
LiquidCrystal_I2CAdapter lcdAdapter(&lcd);
CharacterDisplayRenderer renderer(&lcdAdapter, 16, 2);
LcdMenu menu(renderer);
KeyboardAdapter keyboard(&menu, &Serial);

// Vars

volatile bool triggerTimeUpdate = false;

void TimeUpdate()

{
    if (triggerTimeUpdate) {
        updateDateTime();
        menu.refresh();
        triggerTimeUpdate = false;
    }
}

void setup() {
    Serial.begin(9600);
    renderer.begin();
    menu.setScreen(mainScreen);
    RTC.begin();

    RTCTime startTime(1, Month::JANUARY, 2024, 0, 0, 0, DayOfWeek::MONDAY,
                      SaveLight::SAVING_TIME_INACTIVE);

    RTC.setTime(startTime);

    updateDateTime();
    menu.refresh();

    RTC.setPeriodicCallback([]() {triggerTimeUpdate = true;}, Period::N4_TIMES_EVERY_SEC);
}

void loop() {
    TimeUpdate();
}

// Functions

// Arrow hiding on main menu