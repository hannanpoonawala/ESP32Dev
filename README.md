# ESP32 Wi-Fi & Bluetooth Scanner with TFT UI

An ESP32-based wireless analyzer that scans **Wi-Fi (2.4 GHz)** and **Bluetooth/BLE** devices and displays results on a **TFT SPI display** with an interactive user interface.  
The project is modular, scalable, and designed for learning embedded systems, wireless protocols, and UI-driven firmware architecture.

---

## 📌 Features

- 📡 **Wi-Fi Scanning**
  - Scan nearby Wi-Fi networks
  - Display SSID, RSSI (signal strength), channel, and encryption type
  - Real-time refresh

- 🔵 **Bluetooth / BLE Scanning**
  - Discover nearby Bluetooth & BLE devices
  - Display device name (if available), MAC address, and RSSI

- 🖥️ **TFT User Interface**
  - Interactive on-device UI
  - Mode switching between Wi-Fi and Bluetooth scan
  - Clear separation between UI and scanning logic

- 🧩 **Modular Firmware Design**
  - Independent Wi-Fi, Bluetooth, and UI modules
  - Shared data types for clean inter-module communication

---

## 🛠️ Hardware Requirements

- ESP32 (ESP32 Dev Module / ESP32-WROOM)
- 2.4" SPI TFT Display (ILI9341 or compatible)
- (Optional) Touch panel
- USB cable for programming

---

## 📚 Software Requirements

- Arduino IDE or PlatformIO
- ESP32 Board Package
- Required Libraries:
  - `WiFi.h`
  - `BluetoothSerial.h` / `BLEDevice.h`
  - `TFT_eSPI`
  - `SPI.h`

> ⚠️ Ensure `TFT_eSPI` is correctly configured for your display in `User_Setup.h`.
