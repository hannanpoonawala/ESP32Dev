#include "ui_manager.h"

MenuItem mainItems[] = {
  { "WiFi Tools", MENU_WIFI },
  { "Settings", MENU_SETTINGS }
};

MenuItem wifiItems[] = {
  { "Waterfall Graph", PAGE_WIFI_WATERFALL },
  { "Beacon Spammer", PAGE_WIFI_SPAM },
  { "Captive Portal", PAGE_WIFI_PORTAL }
};

UIManager::UIManager(WiFiHandler &wh) : wifi(wh), tft(TFT_eSPI()) {}

void UIManager::begin() {
    tft.init();
    tft.setRotation(0);
    tft.setTouch(calDataPort);
    tft.fillScreen(FLIPPER_BLACK);
}

void UIManager::update() {
    switch (currentState) {
        case MENU_MAIN:
            if (stateChanged) { drawListMenu("Dashboard", mainItems, 2, MENU_MAIN); stateChanged = false; }
            handleListTouch(mainItems, 2, MENU_MAIN);
            break;

        case MENU_WIFI:
            if (stateChanged) { drawListMenu("WiFi Tools", wifiItems, 3, MENU_MAIN); stateChanged = false; }
            handleListTouch(wifiItems, 3, MENU_MAIN);
            break;

        case PAGE_WIFI_WATERFALL:
            if (stateChanged) { wifi.startSniffer(1); drawStatusPage("Waterfall", "Sniffing..."); stateChanged = false; }
            drawWaterfallPage();
            handleListTouch(NULL, 0, MENU_WIFI);
            break;

        case PAGE_WIFI_SPAM:
            if (stateChanged) { wifi.startBeaconSpammer("Flipper_ESP32"); drawStatusPage("Spammer", "Active..."); stateChanged = false; }
            handleListTouch(NULL, 0, MENU_WIFI);
            break;

        case PAGE_WIFI_PORTAL:
            if (stateChanged) { wifi.startCaptivePortal("ESP32_Portal"); drawStatusPage("Portal", "AP: ESP32_Portal"); stateChanged = false; }
            wifi.handlePortal();
            handleListTouch(NULL, 0, MENU_WIFI);
            break;
    }
}

void UIManager::drawListMenu(const char* title, MenuItem items[], int count, MenuState parentState) {
    tft.fillScreen(FLIPPER_BLACK);
    tft.fillRect(0, 0, tft.width(), HEADER_HEIGHT, FLIPPER_ORANGE);
    tft.setTextColor(FLIPPER_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(title, tft.width() / 2, HEADER_HEIGHT / 2);

    for (int i = 0; i < count; i++) {
        int y = HEADER_HEIGHT + 10 + (i * (BUTTON_HEIGHT + BUTTON_GAP));
        tft.drawRoundRect(10, y, tft.width() - 20, BUTTON_HEIGHT, 6, FLIPPER_ORANGE);
        tft.setTextColor(FLIPPER_ORANGE);
        tft.drawString(items[i].label, tft.width() / 2, y + (BUTTON_HEIGHT / 2));
    }

    if (currentState != MENU_MAIN) {
        tft.fillRoundRect(5, tft.height() - 35, 60, 30, 4, FLIPPER_ORANGE);
        tft.setTextColor(FLIPPER_BLACK);
        tft.drawString("BACK", 35, tft.height() - 20);
    }
}

void UIManager::drawStatusPage(const char* title, const char* status) {
    tft.fillScreen(FLIPPER_BLACK);
    tft.fillRect(0, 0, tft.width(), HEADER_HEIGHT, FLIPPER_ORANGE);
    tft.setTextColor(FLIPPER_BLACK);
    tft.drawString(title, tft.width() / 2, HEADER_HEIGHT / 2);
    tft.setTextColor(FLIPPER_WHITE);
    tft.drawString(status, tft.width() / 2, tft.height() / 2);
    tft.fillRoundRect(5, tft.height() - 35, 60, 30, 4, FLIPPER_ORANGE);
    tft.setTextColor(FLIPPER_BLACK);
    tft.drawString("BACK", 35, tft.height() - 20);
}

void UIManager::drawWaterfallPage() {
    int rssi; uint32_t count;
    wifi.getLatestStats(rssi, count);
    static int x = 0;
    int y = map(rssi, -100, -20, tft.height() - 50, HEADER_HEIGHT + 20);
    tft.drawPixel(x, y, FLIPPER_ORANGE);
    x++; if (x > tft.width()) { x = 0; tft.fillRect(0, HEADER_HEIGHT + 1, tft.width(), tft.height() - 80, FLIPPER_BLACK); }
}

void UIManager::handleListTouch(MenuItem items[], int count, MenuState parentState) {
    uint16_t tx, ty;
    if (tft.getTouch(&tx, &ty)) {
        if (tx < 80 && ty > tft.height() - 50) {
            wifi.stopAll(); // STOP WIFI WHEN GOING BACK
            currentState = parentState;
            stateChanged = true;
            delay(200);
            return;
        }
        for (int i = 0; i < count; i++) {
            int yPos = HEADER_HEIGHT + 10 + (i * (BUTTON_HEIGHT + BUTTON_GAP));
            if (ty > yPos && ty < yPos + BUTTON_HEIGHT) {
                currentState = items[i].targetState;
                stateChanged = true;
                delay(200);
            }
        }
    }
}