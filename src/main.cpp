#include <Button.h>
#include <EEPROM.h>
#include <ItemBack.h>
#include <ItemBool.h>
#include <ItemCommand.h>
#include <ItemLabel.h>
#include <ItemList.h>
#include <ItemRange.h>
#include <ItemSubMenu.h>
#include <ItemValue.h>
#include <LcdMenu.h>
#include <MenuScreen.h>
#include <RTC.h>
#include <display/LiquidCrystal_I2CAdapter.h>
#include <input/ButtonAdapter.h>
#include <input/KeyboardAdapter.h>
#include <renderer/CharacterDisplayRenderer.h>

#include <vector>

char timeBuffer[9];
char dateBuffer[11];

// Forward declaration for the change date screen so we can reference it from
// the main menu initializer (the MENU_SCREEN macro emits an extern for the
// screen variable when it is used later in the file).
extern MenuScreen* changeDateScreen;

// Editable backing variables (used with ITEM_*_REF macros)
int editHour = 0;
int editMinute = 0;
int editSecond = 0;
int editDay = 1;
int editYear = 2024;
uint8_t editMonth = 0;  // 0..11 internal index for monthNames

static std::vector<const char*> monthNames = {"Jan", "Feb", "Mar", "Apr",
                                              "May", "Jun", "Jul", "Aug",
                                              "Sep", "Oct", "Nov", "Dec"};

// Populate the editable variables from the RTC
void populateEditFromRTC() {
    RTCTime now;
    RTC.getTime(now);
    editHour = now.getHour();
    editMinute = now.getMinutes();
    editSecond = now.getSeconds();
    editDay = now.getDayOfMonth();
    editYear = now.getYear();
    // Month2int already returns a zero-based month index in this project
    editMonth = Month2int(now.getMonth());
    // Ensure values are in valid ranges (defensive)
    if (editHour < 0) editHour = 0;
    if (editHour > 23) editHour = 23;
    if (editMinute < 0) editMinute = 0;
    if (editMinute > 59) editMinute = 59;
    if (editSecond < 0) editSecond = 0;
    if (editSecond > 59) editSecond = 59;
    if (editMonth > 11) editMonth = 11;
    if (editMonth < 0) editMonth = 0;
    if (editYear < 2000) editYear = 2000;
}

// Return number of days for zero-based month (0..11) and year
static int daysInMonth(uint8_t monthZeroIndex, int year) {
    static const uint8_t mdays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int days = mdays[monthZeroIndex % 12];
    // Leap year check for February
    if (monthZeroIndex == 1) {
        bool leap = (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
        if (leap) days = 29;
    }
    return days;
}

// Clamp edited values into valid ranges before saving
static void clampEditedValues() {
    if (editHour < 0) editHour = 0;
    if (editHour > 23) editHour = 23;
    if (editMinute < 0) editMinute = 0;
    if (editMinute > 59) editMinute = 59;
    if (editSecond < 0) editSecond = 0;
    if (editSecond > 59) editSecond = 59;
    if (editMonth > 11) editMonth = 11;
    if (editMonth < 0) editMonth = 0;
    if (editYear < 2000) editYear = 2000;
    if (editYear > 2099) editYear = 2099;
    int mdays = daysInMonth(static_cast<uint8_t>(editMonth), editYear);
    if (editDay < 1) editDay = 1;
    if (editDay > mdays) editDay = mdays;
}

// Forward-declare save function used by the menu callback; definition comes
// after `menu` is created so it can access that global.
void saveEditedDate();
// Forward-declare update helper used by menu callbacks
void updateDateTime();

// EEPROM storage layout - small struct to persist user-set date/time
struct SavedDateTime {
    uint8_t magic;  // set to SAVE_MAGIC if record is valid
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t day;
    uint8_t month;  // 1..12
    uint16_t year;
};

static const uint8_t SAVE_MAGIC = 0xA5;
static const int EEPROM_ADDR = 0;  // starting address

void saveToEEPROM() {
    SavedDateTime s;
    s.magic = SAVE_MAGIC;
    s.hour = static_cast<uint8_t>(editHour);
    s.minute = static_cast<uint8_t>(editMinute);
    s.second = static_cast<uint8_t>(editSecond);
    s.day = static_cast<uint8_t>(editDay);
    s.month = static_cast<uint8_t>(editMonth);  // store 0..11 (zero-indexed)
    s.year = static_cast<uint16_t>(editYear);
    EEPROM.put(EEPROM_ADDR, s);
    // Read back immediately and print to help debugging persistent storage
    SavedDateTime check;
    EEPROM.get(EEPROM_ADDR, check);
    Serial.print("Saved date/time to EEPROM: ");
    Serial.print(check.hour);
    Serial.print(":" );
    Serial.print(check.minute);
    Serial.print(":" );
    Serial.print(check.second);
    Serial.print(" ");
    Serial.print(check.day);
    Serial.print("/");
    Serial.print(check.month);
    Serial.print("/");
    Serial.println(check.year);
}

void updateDateTime() {
    RTCTime now;
    RTC.getTime(now);

    snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d:%02d", now.getHour(),
             now.getMinutes(), now.getSeconds());
    snprintf(dateBuffer, sizeof(dateBuffer), "%02d/%02d/%04d",
             Month2int(now.getMonth()), now.getDayOfMonth(), now.getYear());
}

bool loadFromEEPROMIfPresent() {
    SavedDateTime s;
    EEPROM.get(EEPROM_ADDR, s);
    if (s.magic != SAVE_MAGIC) return false;
    Serial.print("Loaded EEPROM raw: ");
    Serial.print(s.hour);
    Serial.print(":" );
    Serial.print(s.minute);
    Serial.print(":" );
    Serial.print(s.second);
    Serial.print(" ");
    Serial.print(s.day);
    Serial.print("/");
    Serial.print(s.month);
    Serial.print("/");
    Serial.println(s.year);
    // populate edit vars and set RTC
    editHour = s.hour;
    editMinute = s.minute;
    editSecond = s.second;
    editDay = s.day;
    // Stored as 0..11 already
    editMonth = s.month;
    editYear = s.year;
    // Ensure loaded values are valid
    clampEditedValues();
    RTCTime now;
    RTC.getTime(now);
    now.setHour(editHour);
    now.setMinute(editMinute);
    now.setSecond(editSecond);
    now.setDayOfMonth(editDay);
    now.setMonthOfYear(static_cast<Month>(s.month));
    now.setYear(editYear);
    RTC.setTime(now);
    updateDateTime();
    Serial.println("Loaded date/time from EEPROM");
    return true;
}

// Main Menu

// A small subclass of ItemSubMenu that updates the displayed date/time
// immediately before switching to the submenu. This ensures the submenu
// reflects the RTC state at the moment it's opened.
class ItemSubMenuWithUpdate : public ItemSubMenu {
   public:
    ItemSubMenuWithUpdate(const char* text, MenuScreen*& screen)
        : ItemSubMenu(text, screen) {}

   protected:
    void handleCommit(LcdMenu* menu) override {
        // Pull latest time from RTC into the buffers used by ITEM_VALUE
        updateDateTime();
        // populate editable vars so the edit widgets show current values
        populateEditFromRTC();
        // Now perform the normal submenu transition
        ItemSubMenu::handleCommit(menu);
    }
};

MENU_SCREEN(mainScreen, mainItems, ITEM_VALUE("Time", timeBuffer),
            ITEM_VALUE("Day", dateBuffer),
            new ItemSubMenuWithUpdate("Change Date", changeDateScreen),
            ITEM_COMMAND("Print Message",
                         []() { Serial.println("Hello, world!"); }));

// Change Date submenu: shows the current time/date (by reference to the
// buffers updated via updateDateTime) and provides a back item.
MENU_SCREEN(
    changeDateScreen, changeDateItems,
    // time components (editable by reference)
    ITEM_RANGE_REF<int>(
        "Hour", editHour, 1, 0, 23, [](const Ref<int> value) {}, "%02d"),
    ITEM_RANGE_REF<int>(
        "Min", editMinute, 1, 0, 59, [](const Ref<int> value) {}, "%02d"),
    ITEM_RANGE_REF<int>(
        "Sec", editSecond, 1, 0, 59, [](const Ref<int> value) {}, "%02d"),
    // date components
    ITEM_RANGE_REF<int>(
        "Day", editDay, 1, 1, 31, [](const Ref<int> value) {}, "%02d"),
    ITEM_LIST_REF(
        "Month", monthNames, [](const Ref<uint8_t> value) {}, editMonth),
    ITEM_RANGE_REF<int>(
        "Year", editYear, 1, 2000, 2099, [](const Ref<int> value) {}, "%04d"),
    ITEM_COMMAND("Save", saveEditedDate), ITEM_BACK("Back"));

LiquidCrystal_I2C lcd(0x20, 16, 2);
LiquidCrystal_I2CAdapter lcdAdapter(&lcd);
CharacterDisplayRenderer renderer(&lcdAdapter, 16, 2);
LcdMenu menu(renderer);
KeyboardAdapter keyboard(&menu, &Serial);

// Buttons
Button upButton(12);
Button downButton(7);
Button enterButton(11);
Button backButton(10);

ButtonAdapter upButtonA(&menu, &upButton, UP, 500, 200);  // hold to repeat
ButtonAdapter downButtonA(&menu, &downButton, DOWN, 500, 200);
ButtonAdapter enterButtonA(&menu, &enterButton, ENTER);
ButtonAdapter backButtonA(&menu, &backButton, BACK);

// Vars

volatile bool triggerUpdate = false;

int light_level = 0;

int light_threshold = 50;

void GetLightLevel(int& level) {
    if (triggerUpdate) {
        level = map(analogRead(A2), 0, 1023, 0, 100);
        Serial.println("Light Level: " + String(level));
    }
}

void TimeUpdate()

{
    if (triggerUpdate) {
        updateDateTime();
        menu.refresh();
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
    // If there's a saved date/time in EEPROM, load it and override the start
    // time
    if (!loadFromEEPROMIfPresent()) {
        updateDateTime();
    }
    menu.refresh();

    RTC.setPeriodicCallback([]() { triggerUpdate = true; },
                            Period::N4_TIMES_EVERY_SEC);

    RTCTime WakeTime;
    RTCTime SleepTime;
    RTCTime ReminderTime;

    // Buttons
    upButton.begin();
    downButton.begin();
    enterButton.begin();
    backButton.begin();
}

void loop() {
    // Observe buttons
    upButtonA.observe();
    downButtonA.observe();
    enterButtonA.observe();
    backButtonA.observe();

    TimeUpdate();
    GetLightLevel(light_level);

    if (triggerUpdate) {
        triggerUpdate = false;
    }
}

// Functions

void saveEditedDate() {
    // Make sure edited values are valid before applying
    clampEditedValues();
    RTCTime now;
    RTC.getTime(now);
    now.setHour(editHour);
    now.setMinute(editMinute);
    now.setSecond(editSecond);
    now.setDayOfMonth(editDay);
    // editMonth is 0..11 (index into monthNames), Month enum starts at 1
    now.setMonthOfYear(static_cast<Month>(editMonth));
    now.setYear(editYear);
    RTC.setTime(now);
    updateDateTime();
    // persist the edited date/time to EEPROM
    saveToEEPROM();
    // return to main screen
    menu.setScreen(mainScreen);
    menu.refresh();
}

// Arrow hiding on main menu