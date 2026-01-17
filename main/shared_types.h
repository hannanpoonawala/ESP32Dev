#ifndef SHARED_TYPES_H
#define SHARED_TYPES_H

#include <Arduino.h>

// Packet types
enum PktType { PKT_MGMT, PKT_DATA, PKT_CTRL, PKT_UNKNOWN };

// WiFi event data structure
struct WiFiEventData {
    char bssid[18];
    int8_t rssi;
    int channel;
    PktType type;
    uint32_t timestamp;
};

// WiFi Network Info (for scanner)
struct WiFiNetwork {
    char ssid[33];
    int8_t rssi;
    uint8_t channel;
    uint8_t encryptionType;
    char bssid[18];
};

// WiFi statistics (lightweight for UI updates)
struct WiFiStats {
    int8_t rssi;
    uint32_t packetCount;
    uint32_t mgmtCount;
    uint32_t dataCount;
    uint32_t ctrlCount;
    int channel;
    bool isActive;
};

// Waterfall data point
struct WaterfallPoint {
    int8_t rssi;
    uint32_t timestamp;
};

// Module states
enum ModuleState {
    STATE_IDLE,
    STATE_SCANNING,
    STATE_SNIFFING,
    STATE_SPAMMING,
    STATE_DEAUTH_DETECT,
    STATE_ERROR
};

// Deauth detection structures
struct DeauthEvent {
    char apMac[18];
    char clientMac[18];
    uint8_t reasonCode;
    uint32_t timestamp;
    int8_t rssi;
};

struct DeauthStats {
    uint32_t totalDeauths;
    uint32_t broadcastDeauths;
    uint32_t suspiciousCount;
    bool attackDetected;
    char suspiciousAP[18];
    uint32_t lastDetectionTime;
};

// UI Update flags (bitwise for efficiency)
#define UI_UPDATE_NONE     0x00
#define UI_UPDATE_STATS    0x01
#define UI_UPDATE_DISPLAY  0x02
#define UI_UPDATE_ALL      0xFF

#endif