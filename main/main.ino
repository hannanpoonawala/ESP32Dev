#include "ESP32Wifi.h"

enum SystemState { MENU, SNIFFING, SPAMMING };
SystemState currentState = MENU;

ESP32Wifi wifi_handler;

void displayMenu() {
    Serial.println("\n========================================");
    Serial.println("      ESP32 WIFI TOOL - MAIN MENU       ");
    Serial.println("========================================");
    Serial.println("[1-13] : Change Channel & Start Sniffing");
    Serial.println("[S]    : Start SSID Spamming Mode");
    Serial.println("[M]    : Return to this Menu");
    Serial.println("[H]    : Toggle Channel Hopping");
    Serial.println("========================================");
    Serial.print("Select a command: ");
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    wifi_handler.begin(1);
    displayMenu();
}

void loop() {
    // 1. Handle Serial Commands
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        input.toUpperCase();

        if (input == "M") {
            currentState = MENU;
            wifi_handler.spamMode = false;
            displayMenu();
        } 
        else if (input == "S") {
            currentState = SPAMMING;
            wifi_handler.spamMode = true;
            Serial.println("\n>>> MODE: SPAMMING ACTIVE (Press 'M' to Stop)");
        } 
        else if (input.toInt() >= 1 && input.toInt() <= 13) {
            uint8_t chan = input.toInt();
            currentState = SNIFFING;
            wifi_handler.spamMode = false;
            esp_wifi_set_channel(chan, WIFI_SECOND_CHAN_NONE);
            Serial.printf("\n>>> MODE: SNIFFING CHANNEL %d (Press 'M' for Menu)\n", chan);
        }
    }

    // 2. State Logic
    switch (currentState) {
        case SNIFFING:
            // The sniffer_callback in the .cpp file handles the printing
            break;

        case SPAMMING:
            // Broadcast fake SSIDs
            wifi_handler.sendBeacon("FBI_SURVEILLANCE");
            delay(10);
            wifi_handler.sendBeacon("FREE_WIFI_STATION");
            delay(100); 
            break;

        case MENU:
            // Do nothing, wait for command
            break;
    }
}