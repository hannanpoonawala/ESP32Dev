#ifndef ESP32_WIFI_H
#define ESP32_WIFI_H

#include "esp_wifi.h"
#include <Arduino.h>

class ESP32Wifi {
public:
    static bool spamMode;
    static void begin(uint8_t channel = 6);
    static void stop();
    
    // The callback must be static to be passed to the ESP-IDF API
    static void sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type);
    static void sendBeacon(const char* ssid);
    
private:
    static void parse_mac_header(uint8_t* payload, uint16_t len);
    static void extract_ssid(uint8_t* payload, uint16_t len);
};

#endif