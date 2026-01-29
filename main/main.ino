#include "wifi_handler.h"
#include "ui_manager.h"

WiFiHandler wifiHandler;
BTHandler btHandler;
UIManager ui(wifiHandler, btHandler);

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