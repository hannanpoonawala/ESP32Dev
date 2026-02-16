#include "wifi_handler.h"
#include "ui_manager.h"

WiFiHandler wifiHandler;
UIManager ui(wifiHandler);

const int pwmPin = 21;
const int pwmChannel = 0;
const int pwmFreq = 5000;
const int pwmResolution = 8;  // 8-bit resolution (0–255)

void setup() {
    Serial.begin(115200);
    
    // Start UI Engine (Core 1 / Standard)
    ui.begin();
    
    Serial.println("System Initialized. Radio on Core 0, UI on Core 1.");
}

void loop() {
    // Keep UI responsive
    ui.update();
    delay(1);
}