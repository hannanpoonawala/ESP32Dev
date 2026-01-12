#ifndef WIFI_HANDLER_H
#define WIFI_HANDLER_H

#include <Arduino.h>
#include <FS.h>
#include <functional>
using fs::FS;  // <--- This line is the "bridge" fix for Core 3.x

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <esp_wifi.h>

struct PacketInfo {
    int8_t rssi;
    uint16_t length;
    int channel;
};

class WiFiHandler {
public:
    WiFiHandler();
    void startSniffer(int channel);
    void getLatestStats(int &rssi, uint32_t &count);
    void startBeaconSpammer(const char* ssid);
    void startCaptivePortal(const char* apName);
    void handlePortal();
    void stopAll();

private:
    static void snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    static int8_t lastRssi;
    static uint32_t packetCount;
    
    WebServer server;
    DNSServer dnsServer;
    bool portalRunning = false;
    bool snifferRunning = false;
    bool spammerRunning = false;
};

#endif