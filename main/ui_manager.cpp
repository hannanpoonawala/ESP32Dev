#include "ui_manager.h"

// --- Menu Definitions ---

// 1. Main Menu Items
MenuItem mainItems[] = {
  { "WiFi", MENU_WIFI },
  { "Bluetooth", MENU_BLUETOOTH },
  { "RFID / NFC", MENU_RFID },
  { "Settings", MENU_SETTINGS }
};
const int MAIN_COUNT = sizeof(mainItems) / sizeof(MenuItem);

// 2. WiFi Menu Items
MenuItem wifiItems[] = { 
    {"Waterfall", PAGE_WATERFALL}, 
    {"Spammer", PAGE_SPAM}, 
    {"Portal", PAGE_PORTAL}
};
const int WIFI_COUNT = sizeof(wifiItems) / sizeof(MenuItem);

// 3. Bluetooth Menu Items
MenuItem btItems[] = {
  { "BT Sniffer", PAGE_BT_TEST }
};
const int BT_COUNT = sizeof(btItems) / sizeof(MenuItem);

// 4. RFID Menu Items
MenuItem rfidItems[] = {
  { "Read Card", PAGE_RFID_SCAN },
  { "Emulate Card", PAGE_RFID_EMIT }
};
const int RFID_COUNT = sizeof(rfidItems) / sizeof(MenuItem);

// 5. Settings Menu Items
MenuItem settingsItems[] = {
  { "Landscape Mode", MENU_MAIN }, // Special logic handles the action
  { "Portrait Mode", MENU_MAIN }   // Special logic handles the action
};
const int SETTINGS_COUNT = sizeof(settingsItems) / sizeof(MenuItem);

UIManager::UIManager(WiFiHandler &wh) : wifi(wh), tft(TFT_eSPI()) {}

void UIManager::begin() {
    // 1. Initialize Backlight hardware
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH); 

    // 2. Initialize Display
    tft.init();
    tft.setRotation(0);

    // 3. Apply Touch Calibration
    tft.setTouch(calDataPort);
    tft.fillScreen(FLIPPER_BLACK);
}

void UIManager::update() {
    switch (currentState) {
        // --- Menus ---
        case MENU_MAIN:
        if (stateChanged) { drawListMenu("Dashboard", mainItems, MAIN_COUNT, MENU_MAIN); stateChanged = false; }
        handleListTouch(mainItems, MAIN_COUNT, MENU_MAIN); 
        break;

        case MENU_WIFI:
        if (stateChanged) { drawListMenu("WiFi Module", wifiItems, WIFI_COUNT, MENU_MAIN); stateChanged = false; }
        handleListTouch(wifiItems, WIFI_COUNT, MENU_MAIN);
        break;

        case MENU_BLUETOOTH:
        if (stateChanged) { drawListMenu("Bluetooth", btItems, BT_COUNT, MENU_MAIN); stateChanged = false; }
        handleListTouch(btItems, BT_COUNT, MENU_MAIN);
        break;

        case MENU_RFID:
        if (stateChanged) { drawListMenu("RFID / NFC", rfidItems, RFID_COUNT, MENU_MAIN); stateChanged = false; }
        handleListTouch(rfidItems, RFID_COUNT, MENU_MAIN);
        break;

        case MENU_SETTINGS:
        if (stateChanged) { drawListMenu("Settings", settingsItems, SETTINGS_COUNT, MENU_MAIN); stateChanged = false; }
        handleSettingsTouch(MENU_MAIN); // Use special handler for rotation logic
        break;

        // --- Placeholder Pages ---
        case PAGE_SPAM:
        if (stateChanged) { drawPlaceholderPage("Scanning...", MENU_WIFI); stateChanged = false; }
        handleListTouch(NULL, 0, MENU_WIFI); // Only listens for back button
        break;
        
        case PAGE_WATERFALL:
        if (stateChanged) { runWaterfall(); stateChanged = false; }
        handleListTouch(NULL, 0, MENU_WIFI);
        break;

        case PAGE_PORTAL:
        if (stateChanged) { drawPlaceholderPage("Packet Mon", MENU_WIFI); stateChanged = false; }
        handleListTouch(NULL, 0, MENU_WIFI);
        break;
        
        case PAGE_BT_TEST:
        if (stateChanged) { drawPlaceholderPage("BT Active", MENU_BLUETOOTH); stateChanged = false; }
        handleListTouch(NULL, 0, MENU_BLUETOOTH);
        break;

        case PAGE_RFID_SCAN:
        if (stateChanged) { drawPlaceholderPage("Reading...", MENU_RFID); stateChanged = false; }
        handleListTouch(NULL, 0, MENU_RFID);
        break;
        
        case PAGE_RFID_EMIT:
        if (stateChanged) { drawPlaceholderPage("Emulating...", MENU_RFID); stateChanged = false; }
        handleListTouch(NULL, 0, MENU_RFID);
        break;
    }
}

void UIManager::drawListMenu(const char* title, MenuItem items[], int count, MenuState parentState) {
    tft.fillScreen(FLIPPER_BLACK);
    
    // 1. Draw Header (Flipper Orange Bar)
    tft.fillRect(0, 0, tft.width(), HEADER_HEIGHT, FLIPPER_ORANGE);
    tft.setTextColor(FLIPPER_BLACK, FLIPPER_ORANGE);
    tft.setTextSize(2); // Standard Font
    tft.setTextDatum(MC_DATUM); // Middle Center
    tft.drawString(title, tft.width() / 2, HEADER_HEIGHT / 2);

    // 2. Draw List Items
    int startY = HEADER_HEIGHT + 10;
    
    for (int i = 0; i < count; i++) {
        // Draw Item Box
        tft.drawRoundRect(10, startY + (i * (BUTTON_HEIGHT + BUTTON_GAP)), tft.width() - 20, BUTTON_HEIGHT, 6, FLIPPER_ORANGE);
        
        // Draw Text
        tft.setTextColor(FLIPPER_ORANGE, FLIPPER_BLACK);
        tft.setTextSize(2);
        tft.setTextDatum(ML_DATUM); // Middle Left
        tft.drawString(items[i].label, 25, startY + (i * (BUTTON_HEIGHT + BUTTON_GAP)) + (BUTTON_HEIGHT / 2));
        
        // Draw Selection Arrow ">"
        tft.drawString(">", tft.width() - 25, startY + (i * (BUTTON_HEIGHT + BUTTON_GAP)) + (BUTTON_HEIGHT / 2));
    }

    // 3. Draw Back Button (If not in Main Menu)
    if (currentState != MENU_MAIN) {
        int backY = tft.height() - 35; // Position at bottom
        tft.fillRoundRect(5, backY, 60, 30, 4, FLIPPER_ORANGE);
        tft.setTextColor(FLIPPER_BLACK, FLIPPER_ORANGE);
        tft.setTextSize(1);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("BACK", 35, backY + 15);
    }
}

void UIManager::drawPlaceholderPage(const char* title, MenuState parentState) {
    tft.fillScreen(FLIPPER_BLACK);
    
    // Header
    tft.fillRect(0, 0, tft.width(), HEADER_HEIGHT, FLIPPER_ORANGE);
    tft.setTextColor(FLIPPER_BLACK, FLIPPER_ORANGE);
    tft.setTextSize(2); 
    tft.setTextDatum(MC_DATUM); 
    tft.drawString(title, tft.width() / 2, HEADER_HEIGHT / 2);

    // Content
    tft.setTextColor(FLIPPER_WHITE, FLIPPER_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Feature", tft.width()/2, tft.height()/2 - 10);
    tft.drawString("Coming Soon", tft.width()/2, tft.height()/2 + 15);
    
    // Back Button
    int backY = tft.height() - 35;
    tft.fillRoundRect(5, backY, 60, 30, 4, FLIPPER_ORANGE);
    tft.setTextColor(FLIPPER_BLACK, FLIPPER_ORANGE);
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("BACK", 35, backY + 15);
}

//============================ LOGIC FUNCTIONS ============================

void UIManager::handleListTouch(MenuItem items[], int count, MenuState parentState) {
    uint16_t x, y;
    
    if (tft.getTouch(&x, &y, 600)) { // Check for touch
        
        // 1. Check "BACK" Button (Bottom Left)
        // Only if we are NOT in the Main Menu
        if (currentState != MENU_MAIN) {
        int backY = tft.height() - 35;
        if (x >= 5 && x <= 65 && y >= backY && y <= backY + 30) {
            currentState = parentState;
            stateChanged = true;
            delay(250); // Debounce
            return;
        }
        }

        // 2. Check List Items (Only if items exist)
        if (items != NULL && count > 0) {
        int startY = HEADER_HEIGHT + 10;
        int itemWidth = tft.width() - 20;

        for (int i = 0; i < count; i++) {
            int itemY = startY + (i * (BUTTON_HEIGHT + BUTTON_GAP));
            
            // Bounding box check
            if (x > 10 && x < (10 + itemWidth) && y > itemY && y < (itemY + BUTTON_HEIGHT)) {
            // Visual Feedback (Invert Colors briefly)
            tft.fillRoundRect(10, itemY, itemWidth, BUTTON_HEIGHT, 6, FLIPPER_ORANGE);
            tft.setTextColor(FLIPPER_BLACK, FLIPPER_ORANGE);
            tft.setTextDatum(ML_DATUM);
            tft.drawString(items[i].label, 25, itemY + (BUTTON_HEIGHT / 2));
            
            delay(150); // Visual feedback delay

            currentState = items[i].targetState;
            stateChanged = true;
            return;
            }
        }
        }
    }
}

void UIManager::handleSettingsTouch(MenuState parentState) {
    uint16_t x, y;
    
    if (tft.getTouch(&x, &y, 600)) {
        // Check Back Button
        int backY = tft.height() - 35;
        if (x >= 5 && x <= 65 && y >= backY && y <= backY + 30) {
        currentState = parentState;
        stateChanged = true;
        delay(250);
        return;
        }

        // Check Settings Items
        int startY = HEADER_HEIGHT + 10;
        int itemWidth = tft.width() - 20;

        // Item 0: Landscape
        int y0 = startY;
        if (x > 10 && x < (10 + itemWidth) && y > y0 && y < (y0 + BUTTON_HEIGHT)) {
            tft.setRotation(1); // Set Landscape
            tft.setTouch(calDataLand);
            tft.fillScreen(FLIPPER_BLACK); // Clear immediately
            stateChanged = true; // Trigger redraw
            delay(250);
        }

        // Item 1: Portrait
        int y1 = startY + (BUTTON_HEIGHT + BUTTON_GAP);
        if (x > 10 && x < (10 + itemWidth) && y > y1 && y < (y1 + BUTTON_HEIGHT)) {
            tft.setRotation(0); // Set Portrait
            tft.setTouch(calDataPort);
            tft.fillScreen(FLIPPER_BLACK); // Clear immediately
            stateChanged = true; // Trigger redraw
            delay(250);
        }
    }
}

void UIManager::runWaterfall() {
    static int x = 0;
    int rssi; uint32_t count;
    wifi.getLatestStats(rssi, count);
    
    // Map RSSI (-100 to -20) to screen height
    int y = map(rssi, -100, -20, tft.height() - 20, 40);
    tft.drawPixel(x, y, FLIPPER_ORANGE);
    
    x++; 
    if (x > tft.width()) { 
        x = 0; 
        tft.fillRect(0, 40, tft.width(), tft.height() - 40, FLIPPER_BLACK); 
    }
}