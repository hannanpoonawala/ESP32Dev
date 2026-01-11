#include "ESP32Wifi.h"

bool ESP32Wifi::spamMode = false;

// Raw 802.11 Beacon Frame Template
uint8_t beacon_raw[] = {
    0x80, 0x00, 0x00, 0x00, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Destination
    0x22, 0x22, 0x22, 0x22, 0x22, 0x22, // Source (Randomized later)
    0x22, 0x22, 0x22, 0x22, 0x22, 0x22, // BSSID
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x64, 0x00, 0x11, 0x00
};

void ESP32Wifi::begin(uint8_t channel) {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
    
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&sniffer_callback);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    
    Serial.printf("Sniffer initialized on Channel %d\n", channel);
}

void ESP32Wifi::sendBeacon(const char* ssid) {
    uint8_t packet[128];
    memcpy(packet, beacon_raw, 36);
    uint8_t ssid_len = strlen(ssid);
    
    // Randomize MAC for each fake SSID
    for(int i=10; i<22; i++) packet[i] = random(256);

    packet[36] = 0x00; // Tag 0: SSID
    packet[37] = ssid_len;
    memcpy(&packet[38], ssid, ssid_len);
    
    uint8_t pos = 38 + ssid_len;
    packet[pos++] = 0x03; packet[pos++] = 0x01;
    packet[pos++] = 1; // Channel

    esp_wifi_80211_tx(WIFI_IF_STA, packet, pos, true);
}

void ESP32Wifi::stop() {
    esp_wifi_set_promiscuous(false);
    esp_wifi_stop();
}

void ESP32Wifi::sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (spamMode) return; // Silent while spamming

    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t*)buf;
    uint8_t *payload = pkt->payload;

    // Table Header (Print only occasionally or on mode change)
    static int count = 0;
    if (count++ % 20 == 0) {
        Serial.println("\n[CH]  [RSSI]  [TYPE]       [SOURCE MAC]");
        Serial.println("------------------------------------------");
    }

    const char* typeStr = (type == 0) ? "MGMT" : (type == 1) ? "CTRL" : "DATA";
    
    Serial.printf("%-4d  %-6d  %-10s  %02X:%02X:%02X:%02X:%02X:%02X\n", 
                  pkt->rx_ctrl.channel, 
                  pkt->rx_ctrl.rssi, 
                  typeStr,
                  payload[10], payload[11], payload[12], payload[13], payload[14], payload[15]);
}

void ESP32Wifi::parse_mac_header(uint8_t* payload, uint16_t len) {
    // Frame Control: Byte 0 & 1
    Serial.printf("FrameCtrl: 0x%02X%02X\n", payload[0], payload[1]);

    // Address 1 (Recipient)
    Serial.printf("Addr1(RX): %02X:%02X:%02X:%02X:%02X:%02X\n", 
                  payload[4], payload[5], payload[6], payload[7], payload[8], payload[9]);

    // Address 2 (Transmitter)
    Serial.printf("Addr2(TX): %02X:%02X:%02X:%02X:%02X:%02X\n", 
                  payload[10], payload[11], payload[12], payload[13], payload[14], payload[15]);

    // Address 3 (Source/BSSID)
    Serial.printf("Addr3(BSS): %02X:%02X:%02X:%02X:%02X:%02X\n", 
                  payload[16], payload[17], payload[18], payload[19], payload[20], payload[21]);
}

void ESP32Wifi::extract_ssid(uint8_t* payload, uint16_t len) {
    // SSID Tag is at offset 36 in standard Beacon frames
    // [24 MAC Header] + [12 Fixed Params] = 36
    if (len > 38) {
        uint8_t tag_num = payload[36];
        uint8_t tag_len = payload[37];
        
        if (tag_num == 0 && (38 + tag_len) <= len) {
            char ssid[33]; // Max 32 chars
            memset(ssid, 0, 33);
            memcpy(ssid, &payload[38], tag_len);
            Serial.printf("SSID: %s\n", ssid);
        }
    }
}