#include "wifi_handler.h"

int8_t WiFiHandler::lastRssi = -100;
uint32_t WiFiHandler::packetCount = 0;

WiFiHandler::WiFiHandler() : server(80) {}

void WiFiHandler::snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    packetCount++;
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    lastRssi = pkt->rx_ctrl.rssi;
}

void WiFiHandler::startSniffer(int channel) {
    stopAll();
    WiFi.mode(WIFI_STA);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&snifferCallback);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

void WiFiHandler::getLatestStats(int &rssi, uint32_t &count) {
    rssi = lastRssi;
    count = packetCount;
}

void WiFiHandler::startBeaconSpammer(const char* ssid) {
    stopAll();
    WiFi.softAP(ssid); // Simplified spammer using AP mode
}

void WiFiHandler::startCaptivePortal(const char* apName) {
    stopAll();
    WiFi.softAP(apName);
    dnsServer.start(53, "*", WiFi.softAPIP());
    server.on("/", [this]() {
        server.send(200, "text/html", "<h1>Flipper ESP32</h1><p>Captive Portal Active</p>");
    });
    server.begin();
    portalRunning = true;
}

void WiFiHandler::handlePortal() {
    if (portalRunning) {
        dnsServer.processNextRequest();
        server.handleClient();
    }
}

void WiFiHandler::stopAll() {
    portalRunning = false;
    esp_wifi_set_promiscuous(false);
    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true);
    server.stop();
    dnsServer.stop();
    packetCount = 0;
}