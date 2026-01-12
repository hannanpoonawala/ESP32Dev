#ifndef UI_MANAGER_H
#define UI_MANAGER_H

// 1. Include the handler first (which contains our FS bridge)
#include "wifi_handler.h" 

// 2. Then include the Display library
#include <TFT_eSPI.h> 
#include <SPI.h>

// Visual Theme
#define FLIPPER_ORANGE 0xFD20 
#define FLIPPER_BLACK  0x0000
#define FLIPPER_WHITE  0xFFFF
#define BUTTON_HEIGHT  40
#define BUTTON_GAP     5
#define HEADER_HEIGHT  30

enum MenuState {
  MENU_MAIN,
  MENU_WIFI,
  PAGE_WIFI_WATERFALL,
  PAGE_WIFI_SPAM,
  PAGE_WIFI_PORTAL,
  MENU_SETTINGS
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

    // Calibration (Portrait)
    uint16_t calDataPort[5] = { 423, 3274, 422, 3384, 4 };

    void drawListMenu(const char* title, MenuItem items[], int count, MenuState parentState);
    void drawWaterfallPage();
    void drawStatusPage(const char* title, const char* status);
    void handleListTouch(MenuItem items[], int count, MenuState parentState);
};

#endif