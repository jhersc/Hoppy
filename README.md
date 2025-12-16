# ESP-12E & ESP32 ↔ RA-02 (SX1278) LoRa Wiring Guide

This guide provides stable wiring configurations to connect either an **ESP-12E (ESP8266, 30-pin)** or an **ESP32 (30-pin)** module to the **RA-02 (SX1278) LoRa module** using SPI.

---

## 1. ESP-12E (ESP8266) Wiring

### Pin Connection Table

| RA-02 Pin | ESP-12E GPIO | ESP-12E Pin Name | Notes |
|-----------|--------------|-----------------|-------|
| VCC       | 3.3V         | 3V3             | **3.3 V only**, ≥120 mA |
| GND       | GND          | GND             | Common ground |
| NSS (CS)  | GPIO15       | D8              | LOW at boot → ideal CS |
| SCK       | GPIO14       | D5              | SPI CLK |
| MOSI      | GPIO13       | D7              | SPI MOSI |
| MISO      | GPIO12       | D6              | SPI MISO |
| RESET     | GPIO16       | D0              | Output only (OK for reset) |
| DIO0      | GPIO5        | D1              | Interrupt (RX/TX done) |
| DIO1      | GPIO4        | D2              | Optional |
| DIO2      | —            | —               | Optional |

### Important Notes

- **Do NOT use GPIO0 or GPIO2** for RA-02 signals (boot-sensitive pins).  
- GPIO16 **cannot generate interrupts**, so avoid using it for DIO0.  
- Ensure a **stable 3.3 V regulator** capable of handling peak currents (~120–140 mA).

### Example Arduino Sketch

```cpp
#include <SPI.h>
#include <LoRa.h>

void setup() {
  SPI.begin(14, 12, 13, 15); // SCK, MISO, MOSI, NSS
  LoRa.setPins(15, 16, 5);   // NSS, RESET, DIO0

  if (!LoRa.begin(433E6)) {  // Adjust frequency as needed
    Serial.println("LoRa init failed!");
    while (1);
  }
  Serial.println("LoRa init OK!");
}

void loop() {
  // Your LoRa code here
}
