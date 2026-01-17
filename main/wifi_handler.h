#ifndef WIFI_HANDLER_H
#define WIFI_HANDLER_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "shared_types.h"

#define MAX_NETWORKS 20
#define WATERFALL_BUFFER_SIZE 80
#define SPAM_SSID_COUNT 10

class WiFiHandler {
public:
    WiFiHandler();
    
    // Lifecycle management
    void begin();
    void stop();
    bool isRunning() const { return running; }
    ModuleState getState() const { return moduleState; }
    
    // ===== SNIFFER MODE =====
    void startSniffer();
    void stopSniffer();
    WiFiStats getStats();
    void resetStats();
    void setChannel(int ch);
    int getChannel() const { return currentChannel; }
    
    // Waterfall data access
    int getWaterfallData(int8_t* buffer, int maxSize);
    
    // ===== SCANNER MODE =====
    void startScan();
    int getNetworkCount() const { return networkCount; }
    WiFiNetwork* getNetworks() { return networks; }
    const char* getAuthTypeName(uint8_t authType);
    
    // ===== BEACON SPAMMER =====
    void startSpammer();
    void stopSpammer();
    bool isSpamming() const { return moduleState == STATE_SPAMMING; }
    const char** getSpamSSIDs() { return spamSSIDs; }
    
    // ===== DEAUTH DETECTOR =====
    void startDeauthDetector();
    void stopDeauthDetector();
    DeauthStats getDeauthStats();
    void resetDeauthStats();

private:
    // Task functions
    static void snifferTask(void* pvParameters);
    static void spammerTask(void* pvParameters);
    static void deauthDetectorTask(void* pvParameters);
    static void snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    static void deauthCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    
    // Task handles
    TaskHandle_t snifferTaskHandle;
    TaskHandle_t spammerTaskHandle;
    TaskHandle_t deauthTaskHandle;
    
    // Queue for packet data
    static QueueHandle_t dataQueue;
    static QueueHandle_t deauthQueue;
    
    // Mutex for thread-safe access
    static SemaphoreHandle_t statsMutex;
    static SemaphoreHandle_t waterfallMutex;
    static SemaphoreHandle_t networkMutex;
    static SemaphoreHandle_t deauthMutex;
    
    // Sniffer statistics (protected by mutex)
    static WiFiStats stats;
    static int currentChannel;
    
    // Waterfall buffer (circular)
    static int8_t waterfallBuffer[WATERFALL_BUFFER_SIZE];
    static int waterfallIndex;
    
    // Scanner data
    WiFiNetwork networks[MAX_NETWORKS];
    int networkCount;
    
    // Spammer SSIDs
    static const char* spamSSIDs[SPAM_SSID_COUNT];
    
    // Deauth detection
    static DeauthStats deauthStats;
    static DeauthEvent deauthHistory[20];
    static int deauthHistoryIndex;
    
    // State
    ModuleState moduleState;
    volatile bool running;
    
    // Helper functions
    void cleanupTasks();
    uint8_t beaconPacket[128];
    void createBeaconFrame(uint8_t* packet, const char* ssid, uint8_t channel);
};

#endif