#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "wifi_handler.h"
#include <TFT_eSPI.h>

// Hardware Constants from your Table
#define TFT_BL 21  // Backlight Pin

// Flipper Theme Colors
#define FLIPPER_ORANGE 0xFD20 
#define FLIPPER_BLACK  0x0000
#define FLIPPER_WHITE  0xFFFF
#define BUTTON_HEIGHT  40
#define BUTTON_GAP     5
#define HEADER_HEIGHT  30

// --- Menu States ---
enum MenuState {
  MENU_MAIN,
  MENU_WIFI,
  MENU_BLUETOOTH,
  MENU_RFID,
  MENU_SETTINGS,
  
  // Leaf Nodes (Placeholders)
  PAGE_SPAM,
  PAGE_WATERFALL,
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
    bool stateChanged = true;
    
    // Calibration data for your specific screen
    uint16_t calDataLand[5] = { 408, 3433, 290, 3447, 7 }; //if tft.setRotation(1)
    uint16_t calDataPort[5] = { 423, 3274, 422, 3384, 4 }; //if tft.setRotation(0)

    void drawListMenu(const char* title, MenuItem items[], int count, MenuState parentState);
    void drawPlaceholderPage(const char* title, MenuState parentState);
    void handleListTouch(MenuItem items[], int count, MenuState parentState);
    void handleSettingsTouch(MenuState parentState);
    void runWaterfall();
};

#endif