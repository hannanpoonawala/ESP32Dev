# ESP32 WiFi Sentinel

A powerful, multi-threaded WiFi security and analysis tool built for the ESP32. Featuring a custom UI inspired by the Flipper Zero, this project enables real-time traffic analysis, network scanning, and security auditing using the ESP32's promiscuous mode.



## 🚀 Features

* **Traffic Analyzer (Waterfall):** Real-time visualization of WiFi traffic across channels 1-13, displaying RSSI strength and packet distribution.
* **WiFi Scanner:** Comprehensive discovery of nearby 2.4GHz networks, including SSID, RSSI, BSSID, Channel, and Encryption type (Open, WEP, WPA, WPA2, WPA3).
* **Beacon Spammer:** Generates multiple fake access points with custom SSIDs (e.g., "FBI Surveillance Van") to test network visibility and client behavior.
* **Deauth Detector:** Monitors for deauthentication attacks in the vicinity, tracking suspicious MAC addresses and attack thresholds to alert the user of potential interference.
* **Dual-Core Architecture:** Optimized for the ESP32; radio operations and packet sniffing run on **Core 0**, while the UI and touch input run on **Core 1** to ensure zero lag during high-traffic analysis.

## 🛠 Hardware Requirements

* **Microcontroller:** ESP32 (DevKit V1 or similar).
* **Display:** SPI-based TFT Display (ILIs9341 or similar, compatible with `TFT_eSPI`).
* **Input:** Resistive or Capacitive Touch screen.
* **Buzzer/LED (Optional):** Indicators for attack detection.

## 📂 Project Structure

* `main.ino`: System entry point, core initialization, and the main UI loop.
* `wifi_handler.cpp/h`: Manages low-level ESP32 WiFi operations, sniffer callbacks, FreeRTOS tasks, and beacon generation.
* `ui_manager.cpp/h`: Handles the menu system, touch calibration, and custom drawing routines for the "Flipper-style" orange and black interface.
* `shared_types.h`: Centralized definitions for packet structures, network info, and system states used across the application.



## ⚙️ Installation & Setup

1.  **Library Prerequisites:**
    Install the following libraries via the Arduino Library Manager:
    * **TFT_eSPI** by Bodmer
    * Standard **WiFi** and **esp_wifi** (included in the ESP32 board package)

2.  **Display Configuration:**
    Navigate to your Arduino libraries folder, open `TFT_eSPI/User_Setup.h`, and configure your display driver and pins. 
    * *Note:* The code assumes the TFT Backlight is connected to **GPIO 21**.

3.  **Flashing:**
    * Open `main.ino` in the Arduino IDE.
    * Select your ESP32 Dev Module.
    * Click **Upload**.

## 🖥 Usage

* **Navigation:** The interface is touch-controlled. Use the main menu to select between WiFi tools and Settings.
* **Traffic ANLZ:** Use the `<` and `>` buttons to sweep different channels. The "Waterfall" graph shows signal density over time.
* **WiFi Scanner:** Tap "SCAN" to populate the list of nearby APs.
* **Settings:** You can toggle between **Landscape** and **Portrait** modes; the UI will automatically recalibrate touch coordinates.

## ⚠️ Disclaimer

This tool is intended for **educational purposes** and **authorized security testing** only. Using this device to perform deauthentication attacks or disrupt networks you do not have explicit permission to test is illegal in many jurisdictions. The developers assume no liability for misuse.

## 📄 License
This project is open-source. Feel free to fork and contribute!
