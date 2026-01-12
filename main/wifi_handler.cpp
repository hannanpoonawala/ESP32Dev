#include "wifi_handler.h"

// Initialize static members
int8_t WiFiHandler::lastRssi = 0;
uint32_t WiFiHandler::packetCount = 0;

WiFiHandler::WiFiHandler() : server(80) {}

// --- 1 & 2. SNIFFER & PROMISCUOUS MODE ---

void WiFiHandler::snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    packetCount++;
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    lastRssi = pkt->rx_ctrl.rssi;
    
    // Here you could parse 'pkt->payload' for deep packet inspection
}

void WiFiHandler::startSniffer(int channel) {
    stopAll();
    snifferRunning = true;
    packetCount = 0;
    
    WiFi.mode(WIFI_STA);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(snifferCallback);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

void WiFiHandler::getLatestStats(int &rssi, uint32_t &count) {
    rssi = lastRssi;
    count = packetCount;
}

// --- 3. BEACON SPAMMER ---

void WiFiHandler::startBeaconSpammer(const char* ssid) {
    stopAll();
    spammerRunning = true;
    WiFi.mode(WIFI_STA);
    
    // Basic 802.11 Beacon Frame template
    uint8_t beaconPacket[128] = {
        0x80, 0x00, 0x00, 0x00,               // Type/Subtype: Beacon
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   // Destination: Broadcast
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06,   // Source MAC (Fake)
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06,   // BSSID (Fake)
        0x00, 0x00,                           // Sequence
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Timestamp
        0x64, 0x00, 0x31, 0x04,               // Beacon Interval / Capability
        0x00, (uint8_t)strlen(ssid)           // SSID Tag
    };
    
    memcpy(&beaconPacket[38], ssid, strlen(ssid));
    size_t packetSize = 38 + strlen(ssid);

    // Note: To keep the UI responsive, usually call this in a timer 
    // or a dedicated Task. For this example, we send one burst.
    esp_wifi_80211_tx(WIFI_IF_STA, beaconPacket, packetSize, true);
}

// --- 4. CAPTIVE PORTAL ---

void WiFiHandler::startCaptivePortal(const char* apName) {
    stopAll();
    portalRunning = true;
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName);
    
    dnsServer.start(53, "*", WiFi.softAPIP());
    
    server.onNotFound([this]() {
        server.send(200, "text/html", "<h1>Multi-Tool Portal</h1><p>Login to continue.</p>");
    });
    
    server.begin();
}

void WiFiHandler::handlePortal() {
    if (portalRunning) {
        dnsServer.processNextRequest();
        server.handleClient();
    }
}

// --- STOP FUNCTION ---

void WiFiHandler::stopAll() {
    if (snifferRunning) {
        esp_wifi_set_promiscuous(false);
        snifferRunning = false;
    }
    
    if (portalRunning) {
        server.stop();
        dnsServer.stop();
        WiFi.softAPdisconnect(true);
        portalRunning = false;
    }

    if (spammerRunning) {
        spammerRunning = false;
    }

    WiFi.disconnect(true);
    delay(100);
}