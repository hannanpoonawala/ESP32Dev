#include "ui_manager.h"
#include "wifi_handler.h"

WiFiHandler wifiHandler;
UIManager ui(wifiHandler);

void setup() {
  Serial.begin(115200);
  ui.begin();
}

void loop() {
  ui.update();
  delay(5);
}