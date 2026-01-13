#include "ui_manager.h"
#include "wifi_handler.h"

// Global instances
WiFiHandler wifiHandler;
UIManager ui(wifiHandler);

void setup() {
    Serial.begin(115200);
    
    ui.begin();
    
    Serial.println("System Online: Hardware Initialized.");
}

void loop() {
    ui.update();
    
    delay(5); 
}