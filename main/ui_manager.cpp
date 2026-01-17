#include "ui_manager.h"

// Menu Definitions
MenuItem mainItems[] = {
    { "WiFi 2.4GHz", MENU_WIFI },
    { "Bluetooth", MENU_BLUETOOTH },
    { "RFID / NFC", MENU_RFID },
    { "Settings", MENU_SETTINGS }
};
const int MAIN_COUNT = sizeof(mainItems) / sizeof(MenuItem);

MenuItem wifiItems[] = { 
    {"Traffic ANLZ", PAGE_WATERFALL}, 
    {"WiFi Scanner", PAGE_SCANNER},
    {"Beacon Spam", PAGE_SPAM},
    {"Deauth Detect", PAGE_DEAUTH}
};
const int WIFI_COUNT = sizeof(wifiItems) / sizeof(MenuItem);

MenuItem btItems[] = {
    { "BT Sniffer", PAGE_BT_TEST }
};
const int BT_COUNT = sizeof(btItems) / sizeof(MenuItem);

MenuItem rfidItems[] = {
    { "Read Card", PAGE_RFID_SCAN },
    { "Emulate Card", PAGE_RFID_EMIT }
};
const int RFID_COUNT = sizeof(rfidItems) / sizeof(MenuItem);

MenuItem settingsItems[] = {
    { "Landscape Mode", MENU_MAIN },
    { "Portrait Mode", MENU_MAIN }
};
const int SETTINGS_COUNT = sizeof(settingsItems) / sizeof(MenuItem);

UIManager::UIManager(WiFiHandler &wh) 
    : wifi(wh), tft(TFT_eSPI()) {
    cachedStats = {-100, 0, 0, 0, 0, 1, false};
}

void UIManager::begin() {
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    tft.init();
    tft.setRotation(0);
    tft.setTouch(calDataPort);
    tft.fillScreen(FLIPPER_BLACK);
    
    wifi.begin();
    lastUpdate = millis();
}

void UIManager::update() {
    switch (currentState) {
        case MENU_MAIN:
            if (stateChanged) { 
                drawListMenu("Dashboard", mainItems, MAIN_COUNT, MENU_MAIN); 
                stateChanged = false; 
            }
            handleListTouch(mainItems, MAIN_COUNT, MENU_MAIN); 
            break;

        case MENU_WIFI:
            if (stateChanged) { 
                drawListMenu("WiFi Module", wifiItems, WIFI_COUNT, MENU_MAIN); 
                stateChanged = false; 
            }
            handleListTouch(wifiItems, WIFI_COUNT, MENU_MAIN);
            break;

        case MENU_BLUETOOTH:
            if (stateChanged) { 
                drawListMenu("Bluetooth", btItems, BT_COUNT, MENU_MAIN); 
                stateChanged = false; 
            }
            handleListTouch(btItems, BT_COUNT, MENU_MAIN);
            break;

        case MENU_RFID:
            if (stateChanged) { 
                drawListMenu("RFID / NFC", rfidItems, RFID_COUNT, MENU_MAIN); 
                stateChanged = false; 
            }
            handleListTouch(rfidItems, RFID_COUNT, MENU_MAIN);
            break;

        case MENU_SETTINGS:
            if (stateChanged) { 
                drawListMenu("Settings", settingsItems, SETTINGS_COUNT, MENU_MAIN); 
                stateChanged = false; 
            }
            handleSettingsTouch(MENU_MAIN);
            break;

        // ===== WiFi Pages =====
        case PAGE_WATERFALL:
            if (stateChanged) {
                drawWaterfallPage();
                stateChanged = false;
            }
            updateWaterfall();
            handleWaterfallTouch();
            break;

        case PAGE_SCANNER:
            if (stateChanged) {
                drawScannerPage();
                wifi.startScan();
                stateChanged = false;
            }
            updateScannerDisplay();
            handleScannerTouch();
            break;

        case PAGE_SPAM:
            if (stateChanged) {
                drawSpammerPage();
                stateChanged = false;
            }
            updateSpammerDisplay();
            handleSpammerTouch();
            break;

        case PAGE_DEAUTH:
            if (stateChanged) {
                drawDeauthPage();
                stateChanged = false;
            }
            updateDeauthDisplay();
            handleDeauthTouch();
            break;

        // ===== Placeholder Pages =====
        case PAGE_PORTAL:
            if (stateChanged) { 
                drawPlaceholderPage("Packet Mon", MENU_WIFI); 
                stateChanged = false; 
            }
            handleListTouch(NULL, 0, MENU_WIFI);
            break;
        
        case PAGE_BT_TEST:
            if (stateChanged) { 
                drawPlaceholderPage("BT Active", MENU_BLUETOOTH); 
                stateChanged = false; 
            }
            handleListTouch(NULL, 0, MENU_BLUETOOTH);
            break;

        case PAGE_RFID_SCAN:
            if (stateChanged) { 
                drawPlaceholderPage("Reading...", MENU_RFID); 
                stateChanged = false; 
            }
            handleListTouch(NULL, 0, MENU_RFID);
            break;
        
        case PAGE_RFID_EMIT:
            if (stateChanged) { 
                drawPlaceholderPage("Emulating...", MENU_RFID); 
                stateChanged = false; 
            }
            handleListTouch(NULL, 0, MENU_RFID);
            break;
    }
}

void UIManager::changeState(MenuState newState) {
    // Cleanup based on current state
    if (currentState == PAGE_WATERFALL) {
        wifi.stopSniffer();
        waterfallRunning = false;
    } else if (currentState == PAGE_SPAM) {
        wifi.stopSpammer();
        spammerRunning = false;
    } else if (currentState == PAGE_DEAUTH) {
        wifi.stopDeauthDetector();
        deauthRunning = false;
    }
    
    previousState = currentState;
    currentState = newState;
    stateChanged = true;
}

bool UIManager::shouldUpdateDisplay() {
    uint32_t now = millis();
    if (now - lastUpdate >= UI_UPDATE_INTERVAL) {
        lastUpdate = now;
        return true;
    }
    return false;
}

uint16_t UIManager::getRssiColor(int8_t rssi) {
    if (rssi >= -45) return 0x07E0; // Green
    if (rssi >= -50) return 0x2FE0;
    if (rssi >= -55) return 0x5FE0;
    if (rssi >= -60) return 0x7FE0;
    if (rssi >= -65) return 0xBFE0;
    if (rssi >= -70) return 0xFFE0; // Yellow
    if (rssi >= -75) return 0xFDE0;
    if (rssi >= -80) return 0xFD20; // Orange
    if (rssi >= -85) return 0xFB00;
    return 0xF800;                  // Red
}

void UIManager::headerUi(const char* title) {
    tft.fillRect(0, 0, tft.width(), HEADER_HEIGHT, FLIPPER_ORANGE);
    tft.setTextColor(FLIPPER_BLACK, FLIPPER_ORANGE);
    tft.setTextSize(2);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(title, tft.width() / 2, HEADER_HEIGHT / 2);
}

void UIManager::backUi(const char* title) {
    int backY = tft.height() - 33;
    tft.fillRoundRect(5, backY, 50, 28, 4, FLIPPER_GREEN);
    tft.setTextColor(FLIPPER_BLACK, FLIPPER_GREEN);
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(title, 30, backY + 14);
}

bool UIManager::handleBackButton() {
    uint16_t x, y;
    if (tft.getTouch(&x, &y, 600)) {
        if (x >= 5 && x <= 55 && y >= tft.height() - 33 && y <= tft.height() - 5) {
            return true;
        }
    }
    return false;
}

void UIManager::drawButton(int x, int y, int w, int h, const char* label, uint16_t color, bool pressed) {
    if (pressed) {
        tft.fillRoundRect(x, y, w, h, 4, color);
        tft.setTextColor(FLIPPER_BLACK, color);
    } else {
        tft.drawRoundRect(x, y, w, h, 4, color);
        tft.setTextColor(color, FLIPPER_BLACK);
    }
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(label, x + w/2, y + h/2);
}

void UIManager::drawBorder(int x, int y, int w, int h, uint16_t color) {
    tft.drawRect(x, y, w, h, color);
}

void UIManager::drawListMenu(const char* title, MenuItem items[], int count, MenuState parentState) {
    tft.fillScreen(FLIPPER_BLACK);
    headerUi(title);

    int startY = HEADER_HEIGHT + 10;
    
    for (int i = 0; i < count; i++) {
        int itemY = startY + (i * (BUTTON_HEIGHT + BUTTON_GAP));
        
        tft.drawRoundRect(10, itemY, tft.width() - 20, BUTTON_HEIGHT, 6, FLIPPER_GREEN);
        tft.setTextColor(FLIPPER_GREEN, FLIPPER_BLACK);
        tft.setTextSize(2);
        tft.setTextDatum(ML_DATUM);
        tft.drawString(items[i].label, 25, itemY + (BUTTON_HEIGHT / 2));
        tft.drawString(">", tft.width() - 25, itemY + (BUTTON_HEIGHT / 2));
    }

    if (currentState != MENU_MAIN) {
        backUi("<<<");
    }
}

void UIManager::drawPlaceholderPage(const char* title, MenuState parentState) {
    tft.fillScreen(FLIPPER_BLACK);
    headerUi(title);

    tft.setTextColor(FLIPPER_WHITE, FLIPPER_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Feature", tft.width()/2, tft.height()/2 - 10);
    tft.drawString("Coming Soon", tft.width()/2, tft.height()/2 + 15);
    
    backUi("<<<");
}

// ==================== WATERFALL PAGE ====================

void UIManager::drawWaterfallPage() {
    tft.fillScreen(FLIPPER_BLACK);
    headerUi("WiFi Traffic");
    
    // Draw borders (1px from header and back button)
    int graphY = HEADER_HEIGHT + 1;
    int graphH = tft.height() - HEADER_HEIGHT - 57;
    int dataY = graphY + graphH + 1;
    drawBorder(0, graphY, tft.width(), graphH, FLIPPER_GRAY);
    tft.drawRoundRect(0, dataY, tft.width(), 20, 6, FLIPPER_GREEN);
    
    // Draw Y-axis labels (RSSI scale)
    tft.setTextColor(FLIPPER_GREEN, FLIPPER_BLACK);
    tft.setTextSize(1);
    tft.setTextDatum(MR_DATUM);
    
    tft.drawString("0", 12, graphY + 10);
    tft.drawString("-50", 18, graphY + graphH / 2);
    tft.drawString("-100", 24, graphY + graphH - 10);
    
    // Draw channel controls
    drawButton(tft.width() - 70, tft.height() - 33, 30, 28, "<", FLIPPER_GREEN);
    drawButton(tft.width() - 35, tft.height() - 33, 30, 28, ">", FLIPPER_GREEN);
    
    // Draw start/stop button
    drawButton(tft.width()/2 - 30, tft.height() - 33, 60, 28, "START", FLIPPER_GREEN);
    
    backUi("<<<");
    waterfallX = 25; // Start drawing position
}

void UIManager::updateWaterfall() {
    if (!shouldUpdateDisplay()) return;
    
    if (waterfallRunning) {
        cachedStats = wifi.getStats();
        
        int graphY = HEADER_HEIGHT + 2;
        int graphH = tft.height() - HEADER_HEIGHT - 59;
        int dataY = graphY + graphH + 2;
        
        int8_t rssi = cachedStats.rssi;
        if (rssi < -100) rssi = -100;
        if (rssi > 0) rssi = 0;
        
        // Map RSSI to Y position
        int y = map(rssi, 0, -100, graphY, graphY + graphH - 1);
        
        // Get color based on signal strength
        uint16_t color = getRssiColor(rssi);
        
        // Draw vertical line from signal to bottom
        for (int i = y; i < graphY + graphH - 1; i++) {
            tft.drawPixel(waterfallX, i, color);
        }
        
        // Update channel/packet info
        tft.drawRoundRect(0, dataY, tft.width(), 20, 6, FLIPPER_GREEN);
        tft.setTextColor(FLIPPER_GREEN, FLIPPER_BLACK);
        tft.setTextSize(1);
        tft.setTextDatum(MC_DATUM);
        
        char info[32];
        snprintf(info, 32, "Ch:%d | Rssi:%d | Pkts:%lu", cachedStats.channel, cachedStats.rssi, cachedStats.packetCount);
        tft.drawString(info, tft.width()/2, HEADER_HEIGHT + graphH + 4 + (20/2));
        
        // Advance waterfall
        waterfallX++;
        if (waterfallX >= tft.width() - 2) {
            waterfallX = 25;
            // Clear graph area
            tft.fillRect(25, graphY, tft.width() - 26, graphH - 1, FLIPPER_BLACK);
        }
    }
}

void UIManager::handleWaterfallTouch() {
    uint16_t x, y;
    
    if (tft.getTouch(&x, &y, 600)) {
        
        // Back button
        if (handleBackButton()) {
            changeState(MENU_WIFI);
            delay(200);
            return;
        }
        
        // Channel decrease
        if (x >= tft.width() - 70 && x <= tft.width() - 40 && y >= tft.height() - 33 && y <= tft.height() - 5) {
            int ch = wifi.getChannel();
            if (ch > 1) wifi.setChannel(ch - 1);
            delay(200);
            return;
        }
        
        // Channel increase
        if (x >= tft.width() - 35 && x <= tft.width() - 5 && y >= tft.height() - 33 && y <= tft.height() - 5) {
            int ch = wifi.getChannel();
            if (ch < 13) wifi.setChannel(ch + 1);
            delay(200);
            return;
        }
        
        // Start/Stop button
        if (x >= tft.width()/2 - 30 && x <= tft.width()/2 + 30 && 
            y >= tft.height() - 33 && y <= tft.height() - 5) {
            
            if (waterfallRunning) {
                wifi.stopSniffer();
                waterfallRunning = false;
                drawButton(tft.width()/2 - 30, tft.height() - 33, 60, 28, "START", FLIPPER_GREEN);
            } else {
                wifi.startSniffer();
                waterfallRunning = true;
                waterfallX = 25;
                int graphY = HEADER_HEIGHT + 2;
                int graphH = tft.height() - HEADER_HEIGHT - 39;
                tft.fillRect(25, graphY, tft.width() - 26, graphH - 1, FLIPPER_BLACK);
                drawButton(tft.width()/2 - 30, tft.height() - 33, 60, 28, "STOP", FLIPPER_GREEN, true);
            }
            
            delay(200);
            return;
        }
    }
}

// ==================== SCANNER PAGE ====================

void UIManager::drawScannerPage() {
    tft.fillScreen(FLIPPER_BLACK);
    headerUi("WiFi Scanner");
    
    // Draw scan button
    drawButton(tft.width()/2 - 40, tft.height() - 33, 80, 28, "SCAN", FLIPPER_GREEN);
    
    backUi("<<<");
    
    // Draw scanning message
    tft.setTextColor(FLIPPER_WHITE, FLIPPER_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Scanning...", tft.width()/2, tft.height()/2);
}

void UIManager::updateScannerDisplay() {
    static bool displayDrawn = false;
    static int lastNetworkCount = -1;
    
    // Only update when scan completes and network count changes
    if (wifi.getState() == STATE_SCANNING) {
        displayDrawn = false;
        return;
    }
    
    int currentCount = wifi.getNetworkCount();
    
    // Prevent flickering by drawing only once after scan
    if (displayDrawn && currentCount == lastNetworkCount) {
        return;
    }
    
    if (currentCount > 0) {
        int listY = HEADER_HEIGHT + 2;
        int listH = tft.height() - HEADER_HEIGHT - 37;
        
        // Clear list area once
        tft.fillRect(1, listY, tft.width() - 2, listH, FLIPPER_BLACK);
        drawBorder(0, listY, tft.width(), listH, FLIPPER_GRAY);
        
        WiFiNetwork* networks = wifi.getNetworks();
        
        tft.setTextSize(1);
        int itemH = 28;
        int maxVisible = listH / itemH;
        
        for (int i = 0; i < min(currentCount, maxVisible); i++) {
            int itemY = listY + 2 + (i * itemH);
            
            // SSID
            tft.setTextColor(FLIPPER_GREEN, FLIPPER_BLACK);
            tft.setTextDatum(TL_DATUM);
            
            String ssid = String(networks[i].ssid);
            if (ssid.length() == 0) ssid = "<Hidden>";
            if (ssid.length() > 18) ssid = ssid.substring(0, 15) + "...";
            tft.drawString(ssid, 5, itemY);
            
            // RSSI with color
            uint16_t rssiColor = getRssiColor(networks[i].rssi);
            tft.setTextColor(rssiColor, FLIPPER_BLACK);
            tft.setTextDatum(TR_DATUM);
            tft.drawString(String(networks[i].rssi) + "dBm", tft.width() - 5, itemY);
            
            // Auth type
            tft.setTextColor(FLIPPER_WHITE, FLIPPER_BLACK);
            tft.setTextDatum(TL_DATUM);
            tft.drawString(wifi.getAuthTypeName(networks[i].encryptionType), 5, itemY + 12);
            
            // Channel
            tft.setTextDatum(TR_DATUM);
            tft.drawString("Ch:" + String(networks[i].channel), tft.width() - 5, itemY + 12);
            
            // Divider line
            if (i < min(currentCount, maxVisible) - 1) {
                tft.drawLine(5, itemY + itemH - 2, tft.width() - 5, itemY + itemH - 2, FLIPPER_GRAY);
            }
        }
        
        displayDrawn = true;
        lastNetworkCount = currentCount;
    }
}

void UIManager::handleScannerTouch() {
    uint16_t x, y;
    
    if (tft.getTouch(&x, &y, 600)) {
        
        // Back button
        if (handleBackButton()) {
            changeState(MENU_WIFI);
            delay(200);
            return;
        }
        
        // Scan button
        if (x >= tft.width()/2 - 40 && x <= tft.width()/2 + 40 && 
            y >= tft.height() - 33 && y <= tft.height() - 5) {
            
            drawButton(tft.width()/2 - 40, tft.height() - 33, 80, 28, "SCAN", FLIPPER_GREEN, true);
            
            // Clear display
            int listY = HEADER_HEIGHT + 2;
            int listH = tft.height() - HEADER_HEIGHT - 37;
            tft.fillRect(1, listY, tft.width() - 2, listH, FLIPPER_BLACK);
            tft.setTextColor(FLIPPER_WHITE, FLIPPER_BLACK);
            tft.setTextDatum(MC_DATUM);
            tft.drawString("Scanning...", tft.width()/2, tft.height()/2);
            
            wifi.startScan();
            
            delay(200);
            drawButton(tft.width()/2 - 40, tft.height() - 33, 80, 28, "SCAN", FLIPPER_GREEN);
            return;
        }
    }
}

// ==================== SPAMMER PAGE ====================

void UIManager::drawSpammerPage() {
    tft.fillScreen(FLIPPER_BLACK);
    headerUi("Beacon Spammer");
    
    // Draw SSID list area
    int listY = HEADER_HEIGHT + 5;
    int listH = tft.height() - HEADER_HEIGHT - 42;
    drawBorder(5, listY, tft.width() - 10, listH, FLIPPER_GRAY);
    
    tft.setTextColor(FLIPPER_WHITE, FLIPPER_BLACK);
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Spamming SSIDs:", tft.width()/2, listY + 8);
    
    // Draw start button
    drawButton(tft.width()/2 - 40, tft.height() - 33, 80, 28, "START", FLIPPER_GREEN);
    
    backUi("<<<");
}

void UIManager::updateSpammerDisplay() {
    static uint32_t lastUpdate = 0;
    
    if (millis() - lastUpdate < 500) return;
    lastUpdate = millis();
    
    int listY = HEADER_HEIGHT + 5;
    int listH = tft.height() - HEADER_HEIGHT - 42;
    
    // Clear list content area
    tft.fillRect(6, listY + 18, tft.width() - 12, listH - 20, FLIPPER_BLACK);
    
    if (spammerRunning) {
        // Show active SSIDs with animation
        static int highlightIndex = 0;
        
        const char** ssids = wifi.getSpamSSIDs();
        tft.setTextSize(1);
        tft.setTextDatum(TL_DATUM);
        
        int startY = listY + 22;
        int itemH = 12;
        
        for (int i = 0; i < 10; i++) {
            int itemY = startY + (i * itemH);
            
            // Highlight current SSID being broadcast
            if (i == highlightIndex) {
                tft.setTextColor(FLIPPER_GREEN, FLIPPER_BLACK);
                tft.drawString(">", 10, itemY);
            } else {
                tft.setTextColor(FLIPPER_GRAY, FLIPPER_BLACK);
            }
            
            String ssid = String(ssids[i]);
            if (ssid.length() > 20) ssid = ssid.substring(0, 17) + "...";
            tft.drawString(ssid, 20, itemY);
        }
        
        highlightIndex = (highlightIndex + 1) % 10;
        
    } else {
        // Show SSID list when idle
        const char** ssids = wifi.getSpamSSIDs();
        tft.setTextSize(1);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(FLIPPER_GRAY, FLIPPER_BLACK);
        
        int startY = listY + 22;
        int itemH = 12;
        
        for (int i = 0; i < 10; i++) {
            int itemY = startY + (i * itemH);
            String ssid = String(ssids[i]);
            if (ssid.length() > 22) ssid = ssid.substring(0, 19) + "...";
            tft.drawString(ssid, 15, itemY);
        }
    }
}

void UIManager::handleSpammerTouch() {
    uint16_t x, y;
    
    if (tft.getTouch(&x, &y, 600)) {
        
        // Back button
        if (handleBackButton()) {
            changeState(MENU_WIFI);
            delay(200);
            return;
        }
        
        // Start/Stop button
        if (x >= tft.width()/2 - 40 && x <= tft.width()/2 + 40 && 
            y >= tft.height() - 33 && y <= tft.height() - 5) {
            
            if (spammerRunning) {
                wifi.stopSpammer();
                spammerRunning = false;
                drawButton(tft.width()/2 - 40, tft.height() - 33, 80, 28, "START", FLIPPER_GREEN);
            } else {
                wifi.startSpammer();
                spammerRunning = true;
                drawButton(tft.width()/2 - 40, tft.height() - 33, 80, 28, "STOP", FLIPPER_GREEN, true);
            }
            
            delay(200);
            return;
        }
    }
}

// ==================== DEAUTH DETECTOR PAGE ====================

void UIManager::drawDeauthPage() {
    tft.fillScreen(FLIPPER_BLACK);
    headerUi("Deauth Detector");
    
    // Draw stats area
    int statsY = HEADER_HEIGHT + 5;
    drawBorder(5, statsY, tft.width() - 10, 60, FLIPPER_GRAY);
    
    // Draw alert area
    int alertY = statsY + 65;
    drawBorder(5, alertY, tft.width() - 10, 40, FLIPPER_GRAY);
    
    // Draw status area
    int statusY = alertY + 45;
    drawBorder(5, statusY, tft.width() - 10, tft.height() - statusY - 38, FLIPPER_GRAY);
    
    // Draw start button
    drawButton(tft.width()/2 - 40, tft.height() - 33, 80, 28, "START", FLIPPER_GREEN);
    
    backUi("<<<");
}

void UIManager::updateDeauthDisplay() {
    if (!shouldUpdateDisplay()) return;
    
    DeauthStats stats = wifi.getDeauthStats();
    
    // Update stats
    int statsY = HEADER_HEIGHT + 5;
    tft.fillRect(6, statsY + 1, tft.width() - 12, 58, FLIPPER_BLACK);
    
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(FLIPPER_GREEN, FLIPPER_BLACK);
    
    char buf[30];
    snprintf(buf, 30, "Total: %lu", stats.totalDeauths);
    tft.drawString(buf, 10, statsY + 5);
    
    snprintf(buf, 30, "Broadcast: %lu", stats.broadcastDeauths);
    tft.drawString(buf, 10, statsY + 18);
    
    snprintf(buf, 30, "Suspicious: %lu", stats.suspiciousCount);
    tft.drawString(buf, 10, statsY + 31);
    
    if (strlen(stats.suspiciousAP) > 0) {
        tft.setTextColor(FLIPPER_ORANGE, FLIPPER_BLACK);
        snprintf(buf, 30, "AP: %.17s", stats.suspiciousAP);
        tft.drawString(buf, 10, statsY + 44);
    }
    
    // Update alert
    int alertY = statsY + 65;
    tft.fillRect(6, alertY + 1, tft.width() - 12, 38, FLIPPER_BLACK);
    tft.setTextDatum(MC_DATUM);
    
    if (stats.attackDetected) {
        tft.setTextColor(FLIPPER_RED, FLIPPER_BLACK);
        tft.setTextSize(2);
        tft.drawString("ATTACK!", tft.width()/2, alertY + 12);
        tft.setTextSize(1);
        tft.drawString("Detected", tft.width()/2, alertY + 28);
    } else if (deauthRunning) {
        tft.setTextColor(FLIPPER_GREEN, FLIPPER_BLACK);
        tft.setTextSize(1);
        tft.drawString("Monitoring...", tft.width()/2, alertY + 20);
    } else {
        tft.setTextColor(FLIPPER_GRAY, FLIPPER_BLACK);
        tft.setTextSize(1);
        tft.drawString("Idle", tft.width()/2, alertY + 20);
    }
    
    // Update status text
    int statusY = alertY + 45;
    tft.fillRect(6, statusY + 1, tft.width() - 12, tft.height() - statusY - 39, FLIPPER_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(1);
    tft.setTextColor(FLIPPER_WHITE, FLIPPER_BLACK);
    
    if (deauthRunning) {
        tft.drawString("Watching for:", 10, statusY + 5);
        tft.setTextColor(FLIPPER_GRAY, FLIPPER_BLACK);
        tft.drawString("- Broadcast deauths", 10, statusY + 18);
        tft.drawString("- Excessive frames", 10, statusY + 30);
        tft.drawString("- Repeated attacks", 10, statusY + 42);
        tft.drawString("- Suspicious APs", 10, statusY + 54);
    }
}

void UIManager::handleDeauthTouch() {
    uint16_t x, y;
    
    if (tft.getTouch(&x, &y, 600)) {
        
        // Back button
        if (handleBackButton()) {
            changeState(MENU_WIFI);
            delay(200);
            return;
        }
        
        // Start/Stop button
        if (x >= tft.width()/2 - 40 && x <= tft.width()/2 + 40 && 
            y >= tft.height() - 33 && y <= tft.height() - 5) {
            
            if (deauthRunning) {
                wifi.stopDeauthDetector();
                deauthRunning = false;
                drawButton(tft.width()/2 - 40, tft.height() - 33, 80, 28, "START", FLIPPER_GREEN);
            } else {
                wifi.resetDeauthStats();
                wifi.startDeauthDetector();
                deauthRunning = true;
                drawButton(tft.width()/2 - 40, tft.height() - 33, 80, 28, "STOP", FLIPPER_GREEN, true);
            }
            
            delay(200);
            return;
        }
    }
}

// ==================== SETTINGS ====================

void UIManager::handleListTouch(MenuItem items[], int count, MenuState parentState) {
    uint16_t x, y;
    
    if (tft.getTouch(&x, &y, 600)) {
        
        // Back button (unified handler)
        if (currentState != MENU_MAIN && handleBackButton()) {
            changeState(parentState);
            delay(200);
            return;
        }

        if (items != NULL && count > 0) {
            int startY = HEADER_HEIGHT + 10;
            int itemWidth = tft.width() - 20;

            for (int i = 0; i < count; i++) {
                int itemY = startY + (i * (BUTTON_HEIGHT + BUTTON_GAP));
                
                if (x > 10 && x < (10 + itemWidth) && 
                    y > itemY && y < (itemY + BUTTON_HEIGHT)) {
                    
                    tft.fillRoundRect(10, itemY, itemWidth, BUTTON_HEIGHT, 6, FLIPPER_GREEN);
                    tft.setTextColor(FLIPPER_BLACK, FLIPPER_GREEN);
                    tft.setTextDatum(ML_DATUM);
                    tft.drawString(items[i].label, 25, itemY + (BUTTON_HEIGHT / 2));
                    
                    delay(150);
                    changeState(items[i].targetState);
                    return;
                }
            }
        }
    }
}

void UIManager::handleSettingsTouch(MenuState parentState) {
    uint16_t x, y;
    
    if (tft.getTouch(&x, &y, 600)) {
        // Back button (unified handler)
        if (handleBackButton()) {
            changeState(parentState);
            delay(200);
            return;
        }

        int startY = HEADER_HEIGHT + 10;
        int itemWidth = tft.width() - 20;

        int y0 = startY;
        if (x > 10 && x < (10 + itemWidth) && y > y0 && y < (y0 + BUTTON_HEIGHT)) {
            tft.setRotation(1);
            tft.setTouch(calDataLand);
            tft.fillScreen(FLIPPER_BLACK);
            stateChanged = true;
            delay(200);
        }

        int y1 = startY + (BUTTON_HEIGHT + BUTTON_GAP);
        if (x > 10 && x < (10 + itemWidth) && y > y1 && y < (y1 + BUTTON_HEIGHT)) {
            tft.setRotation(0);
            tft.setTouch(calDataPort);
            tft.fillScreen(FLIPPER_BLACK);
            stateChanged = true;
            delay(200);
        }
    }
}