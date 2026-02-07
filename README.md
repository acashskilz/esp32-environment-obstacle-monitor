# ESP32 Environment & Obstacle Monitor

This repository contains a firmware implementation for the ESP32 using the ESP-IDF framework. The project demonstrates low-level hardware communication, manual protocol implementation, and real-time event handling.

# 🛠 Technical Features
* **Custom 1-Wire Protocol:** Developed a manual bit-banging driver for the DHT11, utilizing microsecond-precision timing (esp_timer) to decode 40-bit data streams.
* **Low-Latency Interrupts:** Engineered an ISR (Interrupt Service Routine) using `IRAM_ATTR` to handle obstacle detection events with real-time response.
* **Robust Data Integrity:** Implemented checksum verification with bitmasking (`&0xFF`) to handle 8-bit integer overflows.
* **Defensive Programming:** Integrated timeout guards in every communication phase to prevent system hangs.

#  Challenges & Solutions
* **Handshake Timing:** Solved DHT11 "Sensor Timeout" issues by recalibrating the handshake signal to 20ms to accommodate generic sensor clones.
* **Checksum Logic:** Debugged a verification failure caused by 8-bit sum overflows; resolved it by implementing a bitwise AND mask to match sensor parity logic.

# 📂 Project Structure
- `main/main.c`: Core application logic and custom drivers.
- `main/CMakeLists.txt`: Build configuration for the main component.

# Working Video
https://github.com/user-attachments/assets/d51ca68c-1644-4c71-8a4b-8156961350be

Component,        Pin,       
DHT11 (Data),    GPIO 4
IR Sensor (Out), GPIO 27
