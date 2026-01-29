#include "bt_handler.h"

// Spam device names and appearances
const BTSpamData BTHandler::spamData[BT_SPAM_COUNT] = {
    {"Apple AirPods Pro", 0x0941},      // Headphones
    {"Galaxy Buds", 0x0942},            // Headphones
    {"Sony WH-1000XM4", 0x0941},        // Headphones
    {"iPhone 15 Pro", 0x0200},          // Phone
    {"Samsung Galaxy S24", 0x0200},     // Phone
    {"Apple Watch Ultra", 0x00C0},      // Watch
    {"Fitbit Charge", 0x0480},          // Fitness tracker
    {"Tile Tracker", 0x0200},           // Tracker
    {"JBL Flip 6", 0x0840},             // Speaker
    {"Microsoft Surface", 0x0080}       // Tablet
};

// Initialize static members
BLEScan* BTHandler::pBLEScan = nullptr;
BTDevice BTHandler::devices[MAX_BT_DEVICES];
int BTHandler::deviceCount = 0;
BTStats BTHandler::stats = {0, 0, 0, false};
SemaphoreHandle_t BTHandler::deviceMutex = NULL;

BTDevice BTHandler::trackers[10];
int BTHandler::trackerCount = 0;
SemaphoreHandle_t BTHandler::trackerMutex = NULL;

int8_t BTHandler::rssiHistory[50];
int BTHandler::rssiIndex = 0;
int8_t BTHandler::strongestRSSI = -100;

BTHandler::BTHandler() 
    : spammerTaskHandle(NULL), moduleState(BT_STATE_IDLE), running(false) {
    
    // Initialize RSSI history
    for (int i = 0; i < 50; i++) {
        rssiHistory[i] = -100;
    }
}

void BTHandler::begin() {
    if (running) return;
    
    // Create mutexes
    if (deviceMutex == NULL) deviceMutex = xSemaphoreCreateMutex();
    if (trackerMutex == NULL) trackerMutex = xSemaphoreCreateMutex();
    
    // Initialize BLE
    BLEDevice::init("ESP32-Flipper");
    
    running = true;
    moduleState = BT_STATE_IDLE;
}

void BTHandler::stop() {
    if (!running) return;
    
    stopScan();
    stopSpammer();
    stopSkimmer();
    
    // Deinitialize BLE
    BLEDevice::deinit(true);
    
    running = false;
    moduleState = BT_STATE_IDLE;
}

void BTHandler::cleanupTasks() {
    if (spammerTaskHandle != NULL) {
        vTaskDelete(spammerTaskHandle);
        spammerTaskHandle = NULL;
    }
}

// ==================== BLE SCANNER ====================

void BTHandler::AdvertisedDeviceCallbacks::onResult(BLEAdvertisedDevice advertisedDevice) {
    if (xSemaphoreTake(deviceMutex, pdMS_TO_TICKS(10))) {
        
        // Check if device already exists
        bool found = false;
        for (int i = 0; i < deviceCount; i++) {
            if (strcmp(devices[i].address, advertisedDevice.getAddress().toString().c_str()) == 0) {
                // Update existing device
                devices[i].rssi = advertisedDevice.getRSSI();
                devices[i].lastSeen = millis();
                found = true;
                break;
            }
        }
        
        // Add new device
        if (!found && deviceCount < MAX_BT_DEVICES) {
            strncpy(devices[deviceCount].address, 
                    advertisedDevice.getAddress().toString().c_str(), 17);
            devices[deviceCount].address[17] = '\0';
            
            if (advertisedDevice.haveName()) {
                strncpy(devices[deviceCount].name, 
                        advertisedDevice.getName().c_str(), 32);
                devices[deviceCount].name[32] = '\0';
                devices[deviceCount].hasName = true;
            } else {
                strcpy(devices[deviceCount].name, "Unknown");
                devices[deviceCount].hasName = false;
            }
            
            devices[deviceCount].rssi = advertisedDevice.getRSSI();
            devices[deviceCount].isBLE = true;
            devices[deviceCount].lastSeen = millis();
            
            // Detect device type
            String deviceName = String(devices[deviceCount].name);
            deviceName.toLowerCase();
            
            if (deviceName.indexOf("phone") >= 0 || deviceName.indexOf("iphone") >= 0 || 
                deviceName.indexOf("galaxy") >= 0 || deviceName.indexOf("pixel") >= 0) {
                devices[deviceCount].deviceType = 1; // Phone
            } else if (deviceName.indexOf("airpod") >= 0 || deviceName.indexOf("buds") >= 0 || 
                       deviceName.indexOf("headphone") >= 0 || deviceName.indexOf("wh-") >= 0) {
                devices[deviceCount].deviceType = 2; // Headset
            } else if (deviceName.indexOf("speaker") >= 0 || deviceName.indexOf("jbl") >= 0 || 
                       deviceName.indexOf("bose") >= 0) {
                devices[deviceCount].deviceType = 3; // Speaker
            } else if (deviceName.indexOf("watch") >= 0 || deviceName.indexOf("band") >= 0) {
                devices[deviceCount].deviceType = 4; // Watch
            } else if (deviceName.indexOf("tile") >= 0 || deviceName.indexOf("airtag") >= 0 || 
                       deviceName.indexOf("tracker") >= 0) {
                devices[deviceCount].deviceType = 5; // Tracker
            } else {
                devices[deviceCount].deviceType = 0; // Unknown
            }
            
            deviceCount++;
            stats.devicesFound++;
            stats.bleDevices++;
        }
        
        // Update RSSI tracking
        if (advertisedDevice.getRSSI() > strongestRSSI) {
            strongestRSSI = advertisedDevice.getRSSI();
        }
        
        rssiHistory[rssiIndex] = advertisedDevice.getRSSI();
        rssiIndex = (rssiIndex + 1) % 50;
        
        xSemaphoreGive(deviceMutex);
    }
}

void BTHandler::startScan() {
    if (moduleState == BT_STATE_SCANNING) return;
    
    stopSpammer();
    stopSkimmer();
    
    // Reset device list
    if (xSemaphoreTake(deviceMutex, portMAX_DELAY)) {
        deviceCount = 0;
        stats.devicesFound = 0;
        stats.bleDevices = 0;
        stats.classicDevices = 0;
        strongestRSSI = -100;
        xSemaphoreGive(deviceMutex);
    }
    
    // Setup BLE scan
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    
    // Start continuous scan
    pBLEScan->start(0, false); // 0 = continuous, false = don't clear results
    
    stats.isScanning = true;
    moduleState = BT_STATE_SCANNING;
}

void BTHandler::stopScan() {
    if (moduleState != BT_STATE_SCANNING) return;
    
    if (pBLEScan != nullptr) {
        pBLEScan->stop();
        pBLEScan->clearResults();
    }
    
    stats.isScanning = false;
    moduleState = BT_STATE_IDLE;
}

BTStats BTHandler::getStats() {
    BTStats localStats;
    
    if (xSemaphoreTake(deviceMutex, pdMS_TO_TICKS(10))) {
        localStats = stats;
        xSemaphoreGive(deviceMutex);
    } else {
        localStats = {0, 0, 0, false};
    }
    
    return localStats;
}

uint8_t BTHandler::detectDeviceType(const char* name) {
    String deviceName = String(name);
    deviceName.toLowerCase();
    
    if (deviceName.indexOf("phone") >= 0 || deviceName.indexOf("iphone") >= 0 || 
        deviceName.indexOf("galaxy") >= 0 || deviceName.indexOf("pixel") >= 0) {
        return 1; // Phone
    } else if (deviceName.indexOf("airpod") >= 0 || deviceName.indexOf("buds") >= 0 || 
               deviceName.indexOf("headphone") >= 0 || deviceName.indexOf("wh-") >= 0) {
        return 2; // Headset
    } else if (deviceName.indexOf("speaker") >= 0 || deviceName.indexOf("jbl") >= 0 || 
               deviceName.indexOf("bose") >= 0) {
        return 3; // Speaker
    } else if (deviceName.indexOf("watch") >= 0 || deviceName.indexOf("band") >= 0) {
        return 4; // Watch
    } else if (deviceName.indexOf("tile") >= 0 || deviceName.indexOf("airtag") >= 0 || 
               deviceName.indexOf("tracker") >= 0) {
        return 5; // Tracker
    }
    
    return 0; // Unknown
}

const char* BTHandler::getDeviceTypeName(uint8_t type) {
    switch(type) {
        case 1: return "Phone";
        case 2: return "Headset";
        case 3: return "Speaker";
        case 4: return "Watch";
        case 5: return "Tracker";
        default: return "Unknown";
    }
}

// ==================== BLE SPAMMER ====================

void BTHandler::startSpammer() {
    if (moduleState == BT_STATE_SPAMMING) return;
    
    stopScan();
    stopSkimmer();
    cleanupTasks();
    
    moduleState = BT_STATE_SPAMMING;
    
    xTaskCreatePinnedToCore(
        spammerTask,
        "BTSpam",
        4096,
        this,
        1,
        &spammerTaskHandle,
        0
    );
}

void BTHandler::stopSpammer() {
    if (moduleState != BT_STATE_SPAMMING) return;
    
    cleanupTasks();
    moduleState = BT_STATE_IDLE;
}

void BTHandler::spammerTask(void* pvParameters) {
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    int deviceIndex = 0;
    
    for (;;) {
        // Set device name and appearance
        BLEDevice::deinit(false);
        delay(50);
        
        BLEDevice::init(spamData[deviceIndex].name);
        pAdvertising = BLEDevice::getAdvertising();
        
        // Build advertisement data
        BLEAdvertisementData advertisementData;
        advertisementData.setName(spamData[deviceIndex].name);
        advertisementData.setAppearance(spamData[deviceIndex].appearance);
        advertisementData.setFlags(0x06); // BR/EDR not supported, LE General Discoverable
        
        pAdvertising->setAdvertisementData(advertisementData);
        pAdvertising->setScanResponseData(advertisementData);
        
        // Start advertising
        pAdvertising->start();
        delay(100);
        pAdvertising->stop();
        
        // Next device
        deviceIndex = (deviceIndex + 1) % BT_SPAM_COUNT;
        
        delay(50);
    }
}

// ==================== TRACKER DETECTOR (SKIMMER) ====================

bool BTHandler::isTracker(BLEAdvertisedDevice device) {
    String name = device.haveName() ? String(device.getName().c_str()) : "";
    name.toLowerCase();
    
    // Check for known tracker keywords
    if (name.indexOf("tile") >= 0 || name.indexOf("airtag") >= 0 || 
        name.indexOf("chipolo") >= 0 || name.indexOf("trackr") >= 0 ||
        name.indexOf("nutfind") >= 0) {
        return true;
    }
    
    // Check for Apple Find My network (AirTag) by UUID
    if (device.haveServiceUUID()) {
        // Apple Find My network service UUID
        BLEUUID findMyUUID("0000XXXX-0000-1000-8000-00805F9B34FB");
        // This is a simplified check - real AirTag detection is more complex
    }
    
    // Check for suspicious behavior (very low transmit interval)
    // Trackers often advertise frequently
    
    return false;
}

void BTHandler::startSkimmer() {
    if (moduleState == BT_STATE_SKIMMING) return;
    
    stopScan();
    stopSpammer();
    
    // Reset tracker list
    if (xSemaphoreTake(trackerMutex, portMAX_DELAY)) {
        trackerCount = 0;
        xSemaphoreGive(trackerMutex);
    }
    
    moduleState = BT_STATE_SKIMMING;
    
    // Use same scanning mechanism but filter for trackers
    startScan();
}

void BTHandler::stopSkimmer() {
    if (moduleState != BT_STATE_SKIMMING) return;
    
    stopScan();
    moduleState = BT_STATE_IDLE;
}

// ==================== SIGNAL STRENGTH MONITOR ====================

int8_t BTHandler::getStrongestRSSI() {
    return strongestRSSI;
}

void BTHandler::getRSSIHistory(int8_t* buffer, int size) {
    int count = min(size, 50);
    
    for (int i = 0; i < count; i++) {
        int idx = (rssiIndex + i) % 50;
        buffer[i] = rssiHistory[idx];
    }
}