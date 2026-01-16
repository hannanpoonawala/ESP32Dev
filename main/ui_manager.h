#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "wifi_handler.h"
#include <TFT_eSPI.h>

// Hardware Constants
#define TFT_BL 21

// Flipper Theme Colors
#define FLIPPER_GREEN  0x07E0
#define FLIPPER_BLACK  0x0000
#define FLIPPER_WHITE  0xFFFF
#define FLIPPER_ORANGE 0xFD20
#define FLIPPER_GRAY   0x7BEF
#define BUTTON_HEIGHT  40
#define BUTTON_GAP     5
#define HEADER_HEIGHT  30

// UI Update timing
#define UI_UPDATE_INTERVAL 50  // ms between updates

// Menu States
enum MenuState {
    MENU_MAIN,
    MENU_WIFI,
    MENU_BLUETOOTH,
    MENU_RFID,
    MENU_SETTINGS,
    
    // WiFi Pages
    PAGE_WATERFALL,
    PAGE_SCANNER,
    PAGE_SPAM,
    
    // Other Pages
    PAGE_PORTAL,
    PAGE_BT_TEST,
    PAGE_RFID_SCAN,
    PAGE_RFID_EMIT
};

struct MenuItem {
    const char* label;
    MenuState targetState;
};

class UIManager {
public:
    UIManager(WiFiHandler &wh);
    void begin();
    void update();

private:
    WiFiHandler &wifi;
    TFT_eSPI tft;
    MenuState currentState = MENU_MAIN;
    MenuState previousState = MENU_MAIN;
    bool stateChanged = true;
    
    // Update timing
    uint32_t lastUpdate = 0;
    
    // Cached data
    WiFiStats cachedStats;
    
    // Waterfall state
    bool waterfallRunning = false;
    int waterfallX = 0;
    
    // Scanner state
    int scannerScroll = 0;
    int selectedNetwork = -1;
    
    // Spammer state
    bool spammerRunning = false;
    
    // Calibration data
    uint16_t calDataLand[5] = { 408, 3433, 290, 3447, 7 };
    uint16_t calDataPort[5] = { 423, 3274, 422, 3384, 4 };

    // Drawing functions
    void headerUi(const char* title);
    void backUi(const char* title);
    void drawListMenu(const char* title, MenuItem items[], int count, MenuState parentState);
    void drawPlaceholderPage(const char* title, MenuState parentState);
    
    // Touch handlers
    void handleListTouch(MenuItem items[], int count, MenuState parentState);
    void handleSettingsTouch(MenuState parentState);
    
    // WiFi Pages
    void drawWaterfallPage();
    void updateWaterfall();
    void handleWaterfallTouch();
    
    void drawScannerPage();
    void updateScannerDisplay();
    void handleScannerTouch();
    
    void drawSpammerPage();
    void updateSpammerDisplay();
    void handleSpammerTouch();
    
    // Helper functions
    void changeState(MenuState newState);
    bool shouldUpdateDisplay();
    uint16_t getRssiColor(int8_t rssi);
    void drawButton(int x, int y, int w, int h, const char* label, uint16_t color, bool pressed = false);
    void drawBorder(int x, int y, int w, int h, uint16_t color);
};

#endif