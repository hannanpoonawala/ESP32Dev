#include "wifi_handler.h"

// Spam SSIDs
const char* WiFiHandler::spamSSIDs[SPAM_SSID_COUNT] = {
    "FBI Surveillance Van",
    "NSA Listening Post",
    "Virus Distribution",
    "Free Virus Here",
    "Click for Malware",
    "COVID-19 Hotspot",
    "5G Tower Control",
    "Mom Click Here",
    "Free iPhone 15",
    "Totally Not A Trap"
};

// Initialize static members
QueueHandle_t WiFiHandler::dataQueue = NULL;
SemaphoreHandle_t WiFiHandler::statsMutex = NULL;
SemaphoreHandle_t WiFiHandler::waterfallMutex = NULL;
SemaphoreHandle_t WiFiHandler::networkMutex = NULL;
WiFiStats WiFiHandler::stats = {-100, 0, 0, 0, 0, 1, false};
int WiFiHandler::currentChannel = 1;
int8_t WiFiHandler::waterfallBuffer[WATERFALL_BUFFER_SIZE];
int WiFiHandler::waterfallIndex = 0;

WiFiHandler::WiFiHandler() 
    : snifferTaskHandle(NULL), spammerTaskHandle(NULL), 
      networkCount(0), moduleState(STATE_IDLE), running(false) {
    
    // Initialize waterfall buffer
    for (int i = 0; i < WATERFALL_BUFFER_SIZE; i++) {
        waterfallBuffer[i] = -100;
    }
}

void WiFiHandler::begin() {
    if (running) return;

    // Create mutexes
    if (statsMutex == NULL) statsMutex = xSemaphoreCreateMutex();
    if (waterfallMutex == NULL) waterfallMutex = xSemaphoreCreateMutex();
    if (networkMutex == NULL) networkMutex = xSemaphoreCreateMutex();

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    running = true;
    moduleState = STATE_IDLE;
}

void WiFiHandler::stop() {
    if (!running) return;

    stopSniffer();
    stopSpammer();
    
    running = false;
    moduleState = STATE_IDLE;
}

void WiFiHandler::cleanupTasks() {
    if (snifferTaskHandle != NULL) {
        vTaskDelete(snifferTaskHandle);
        snifferTaskHandle = NULL;
    }
    
    if (spammerTaskHandle != NULL) {
        vTaskDelete(spammerTaskHandle);
        spammerTaskHandle = NULL;
    }
    
    if (dataQueue != NULL) {
        vQueueDelete(dataQueue);
        dataQueue = NULL;
    }
}

// ==================== SNIFFER MODE ====================

void WiFiHandler::startSniffer() {
    if (moduleState == STATE_SNIFFING) return;
    
    stopSpammer(); // Stop spammer if running
    cleanupTasks();

    // Create queue
    dataQueue = xQueueCreate(50, sizeof(WiFiEventData));

    // Reset statistics
    if (xSemaphoreTake(statsMutex, portMAX_DELAY)) {
        stats.rssi = -100;
        stats.packetCount = 0;
        stats.mgmtCount = 0;
        stats.dataCount = 0;
        stats.ctrlCount = 0;
        stats.channel = currentChannel;
        stats.isActive = true;
        xSemaphoreGive(statsMutex);
    }

    // Reset waterfall
    if (xSemaphoreTake(waterfallMutex, portMAX_DELAY)) {
        waterfallIndex = 0;
        for (int i = 0; i < WATERFALL_BUFFER_SIZE; i++) {
            waterfallBuffer[i] = -100;
        }
        xSemaphoreGive(waterfallMutex);
    }

    // Start sniffer task
    xTaskCreatePinnedToCore(
        snifferTask, 
        "WiFiSniff", 
        4096, 
        this,
        2,
        &snifferTaskHandle, 
        0
    );

    moduleState = STATE_SNIFFING;
}

void WiFiHandler::stopSniffer() {
    if (moduleState != STATE_SNIFFING) return;

    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(NULL);

    cleanupTasks();

    if (xSemaphoreTake(statsMutex, portMAX_DELAY)) {
        stats.isActive = false;
        xSemaphoreGive(statsMutex);
    }

    moduleState = STATE_IDLE;
}

WiFiStats WiFiHandler::getStats() {
    WiFiStats localStats;
    
    if (xSemaphoreTake(statsMutex, pdMS_TO_TICKS(10))) {
        localStats = stats;
        xSemaphoreGive(statsMutex);
    } else {
        localStats = {-100, 0, 0, 0, 0, currentChannel, false};
    }
    
    return localStats;
}

void WiFiHandler::resetStats() {
    if (xSemaphoreTake(statsMutex, portMAX_DELAY)) {
        stats.packetCount = 0;
        stats.mgmtCount = 0;
        stats.dataCount = 0;
        stats.ctrlCount = 0;
        xSemaphoreGive(statsMutex);
    }
}

void WiFiHandler::setChannel(int ch) {
    if (ch < 1 || ch > 13) return;
    
    currentChannel = ch;
    
    if (moduleState == STATE_SNIFFING) {
        esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
    }
    
    if (xSemaphoreTake(statsMutex, portMAX_DELAY)) {
        stats.channel = currentChannel;
        xSemaphoreGive(statsMutex);
    }
}

int WiFiHandler::getWaterfallData(int8_t* buffer, int maxSize) {
    int count = min(maxSize, WATERFALL_BUFFER_SIZE);
    
    if (xSemaphoreTake(waterfallMutex, pdMS_TO_TICKS(10))) {
        for (int i = 0; i < count; i++) {
            int idx = (waterfallIndex + i) % WATERFALL_BUFFER_SIZE;
            buffer[i] = waterfallBuffer[idx];
        }
        xSemaphoreGive(waterfallMutex);
        return count;
    }
    
    return 0;
}

void WiFiHandler::snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    WiFiEventData data;

    data.rssi = pkt->rx_ctrl.rssi;
    data.channel = currentChannel;
    data.timestamp = millis();
    
    if (type == WIFI_PKT_MGMT) data.type = PKT_MGMT;
    else if (type == WIFI_PKT_DATA) data.type = PKT_DATA;
    else data.type = PKT_CTRL;

    uint8_t* payload = pkt->payload;
    snprintf(data.bssid, 18, "%02X:%02X:%02X:%02X:%02X:%02X", 
             payload[10], payload[11], payload[12], 
             payload[13], payload[14], payload[15]);

    xQueueSendFromISR(dataQueue, &data, NULL);
}

void WiFiHandler::snifferTask(void* pvParameters) {
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&snifferCallback);
    esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);

    WiFiEventData receivedData;
    
    for (;;) {
        if (xQueueReceive(dataQueue, &receivedData, pdMS_TO_TICKS(100))) {
            
            // Update statistics
            if (xSemaphoreTake(statsMutex, pdMS_TO_TICKS(5))) {
                stats.rssi = receivedData.rssi;
                stats.packetCount++;
                
                switch (receivedData.type) {
                    case PKT_MGMT: stats.mgmtCount++; break;
                    case PKT_DATA: stats.dataCount++; break;
                    case PKT_CTRL: stats.ctrlCount++; break;
                    default: break;
                }
                
                xSemaphoreGive(statsMutex);
            }

            // Update waterfall buffer
            if (xSemaphoreTake(waterfallMutex, pdMS_TO_TICKS(5))) {
                waterfallBuffer[waterfallIndex] = receivedData.rssi;
                waterfallIndex = (waterfallIndex + 1) % WATERFALL_BUFFER_SIZE;
                xSemaphoreGive(waterfallMutex);
            }
        }
        
        vTaskDelay(1);
    }
}

// ==================== SCANNER MODE ====================

void WiFiHandler::startScan() {
    if (moduleState == STATE_SCANNING) return;
    
    stopSniffer();
    stopSpammer();
    
    moduleState = STATE_SCANNING;
    
    // Perform WiFi scan
    int n = WiFi.scanNetworks(false, false, false, 300);
    
    if (xSemaphoreTake(networkMutex, portMAX_DELAY)) {
        networkCount = min(n, MAX_NETWORKS);
        
        for (int i = 0; i < networkCount; i++) {
            strncpy(networks[i].ssid, WiFi.SSID(i).c_str(), 32);
            networks[i].ssid[32] = '\0';
            networks[i].rssi = WiFi.RSSI(i);
            networks[i].channel = WiFi.channel(i);
            networks[i].encryptionType = WiFi.encryptionType(i);
            
            uint8_t* bssid = WiFi.BSSID(i);
            snprintf(networks[i].bssid, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
                     bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
        }
        
        xSemaphoreGive(networkMutex);
    }
    
    WiFi.scanDelete();
    moduleState = STATE_IDLE;
}

const char* WiFiHandler::getAuthTypeName(uint8_t authType) {
    switch(authType) {
        case WIFI_AUTH_OPEN: return "OPEN";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-E";
        case WIFI_AUTH_WPA3_PSK: return "WPA3";
        default: return "UNKN";
    }
}

// ==================== BEACON SPAMMER ====================

void WiFiHandler::createBeaconFrame(uint8_t* packet, const char* ssid, uint8_t channel) {
    int ssidLen = strlen(ssid);
    if (ssidLen > 32) ssidLen = 32;
    
    uint8_t beacon[] = {
        0x80, 0x00,                         // Frame Control
        0x00, 0x00,                         // Duration
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Destination (broadcast)
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // Source (random MAC)
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // BSSID (same as source)
        0x00, 0x00,                         // Sequence Control
        
        // Fixed parameters (12 bytes)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Timestamp
        0x64, 0x00,                         // Beacon interval
        0x31, 0x04,                         // Capability info
        
        // Tagged parameters
        0x00, (uint8_t)ssidLen              // SSID parameter set (tag: 0, length)
    };
    
    memcpy(packet, beacon, sizeof(beacon));
    int pos = sizeof(beacon);
    
    // Copy SSID
    memcpy(packet + pos, ssid, ssidLen);
    pos += ssidLen;
    
    // Supported rates
    packet[pos++] = 0x01; // Tag
    packet[pos++] = 0x08; // Length
    packet[pos++] = 0x82; packet[pos++] = 0x84;
    packet[pos++] = 0x8b; packet[pos++] = 0x96;
    packet[pos++] = 0x0c; packet[pos++] = 0x12;
    packet[pos++] = 0x18; packet[pos++] = 0x24;
    
    // DS Parameter set
    packet[pos++] = 0x03; // Tag
    packet[pos++] = 0x01; // Length
    packet[pos++] = channel;
}

void WiFiHandler::startSpammer() {
    if (moduleState == STATE_SPAMMING) return;
    
    stopSniffer();
    cleanupTasks();
    
    moduleState = STATE_SPAMMING;
    
    xTaskCreatePinnedToCore(
        spammerTask,
        "BeaconSpam",
        4096,
        this,
        1,
        &spammerTaskHandle,
        0
    );
}

void WiFiHandler::stopSpammer() {
    if (moduleState != STATE_SPAMMING) return;
    
    esp_wifi_set_promiscuous(false);
    cleanupTasks();
    
    moduleState = STATE_IDLE;
}

void WiFiHandler::spammerTask(void* pvParameters) {
    WiFiHandler* handler = (WiFiHandler*)pvParameters;
    
    esp_wifi_set_promiscuous(true);
    
    uint8_t packet[128];
    int ssidIndex = 0;
    
    for (;;) {
        // Create and send beacon for current SSID
        handler->createBeaconFrame(packet, spamSSIDs[ssidIndex], currentChannel);
        esp_wifi_80211_tx(WIFI_IF_STA, packet, 128, false);
        
        // Move to next SSID
        ssidIndex = (ssidIndex + 1) % SPAM_SSID_COUNT;
        
        // Small delay between packets
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}