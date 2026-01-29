#ifndef BT_HANDLER_H
#define BT_HANDLER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#define MAX_BT_DEVICES 20
#define BT_SPAM_COUNT 10

// Bluetooth device info
struct BTDevice {
    char address[18];
    char name[33];
    int8_t rssi;
    bool isBLE;
    bool hasName;
    uint32_t lastSeen;
    uint8_t deviceType; // 0=Unknown, 1=Phone, 2=Headset, 3=Speaker, 4=Watch, 5=Tracker
};

// Bluetooth statistics
struct BTStats {
    uint32_t devicesFound;
    uint32_t bleDevices;
    uint32_t classicDevices;
    bool isScanning;
};

// Bluetooth spam data
struct BTSpamData {
    const char* name;
    uint16_t appearance;
};

// Bluetooth module states
enum BTModuleState {
    BT_STATE_IDLE,
    BT_STATE_SCANNING,
    BT_STATE_SPAMMING,
    BT_STATE_SKIMMING,
    BT_STATE_ERROR
};

class BTHandler {
public:
    BTHandler();
    
    // Lifecycle
    void begin();
    void stop();
    bool isRunning() const { return running; }
    BTModuleState getState() const { return moduleState; }
    
    // ===== BLE SCANNER =====
    void startScan();
    void stopScan();
    int getDeviceCount() const { return deviceCount; }
    BTDevice* getDevices() { return devices; }
    BTStats getStats();
    const char* getDeviceTypeName(uint8_t type);
    
    // ===== BLE SPAMMER =====
    void startSpammer();
    void stopSpammer();
    bool isSpamming() const { return moduleState == BT_STATE_SPAMMING; }
    const BTSpamData* getSpamData() { return spamData; }
    
    // ===== AirTag/Tile DETECTOR (Skimmer) =====
    void startSkimmer();
    void stopSkimmer();
    int getTrackerCount() const { return trackerCount; }
    BTDevice* getTrackers() { return trackers; }
    
    // ===== SIGNAL STRENGTH MONITOR =====
    int8_t getStrongestRSSI();
    void getRSSIHistory(int8_t* buffer, int size);

private:
    // BLE Scanning
    static BLEScan* pBLEScan;
    static BTDevice devices[MAX_BT_DEVICES];
    static int deviceCount;
    static BTStats stats;
    static SemaphoreHandle_t deviceMutex;
    
    // BLE Spamming
    static const BTSpamData spamData[BT_SPAM_COUNT];
    TaskHandle_t spammerTaskHandle;
    static void spammerTask(void* pvParameters);
    
    // Tracker detection
    static BTDevice trackers[10];
    static int trackerCount;
    static SemaphoreHandle_t trackerMutex;
    
    // RSSI monitoring
    static int8_t rssiHistory[50];
    static int rssiIndex;
    static int8_t strongestRSSI;
    
    // State
    BTModuleState moduleState;
    volatile bool running;
    
    // Callbacks
    class AdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    public:
        void onResult(BLEAdvertisedDevice advertisedDevice);
    };
    
    // Helper functions
    void cleanupTasks();
    uint8_t detectDeviceType(const char* name);
    bool isTracker(BLEAdvertisedDevice device);
};

#endif