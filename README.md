# ESP-12E & ESP32 ↔ RA-02 (SX1278) LoRa Wiring Guide

This guide provides stable wiring configurations to connect either an **ESP-12E (ESP8266, 30-pin)** or an **ESP32 (30-pin)** module to the **RA-02 (SX1278) LoRa module** using SPI.

---

## 1. ESP-12E (ESP8266) Wiring

### Pin Connection Table

| RA-02 Pin | ESP-12E GPIO | D Label | Notes                                          |
| --------- | ------------ | ------- | ---------------------------------------------- |
| VCC       | 3.3V         | 3V3     | Stable 3.3V ≥120 mA                            |
| GND       | GND          | GND     | Common ground                                  |
| NSS / CS  | GPIO15       | D8      | Boot-safe CS                                   |
| SCK       | GPIO14       | D5      | SPI clock                                      |
| MOSI      | GPIO13       | D7      | SPI MOSI                                       |
| MISO      | GPIO12       | D6      | SPI MISO                                       |
| RESET     | GPIO2        | D4      | Output only; safe boot pin                     |
| DIO0      | GPIO0        | D3      | LoRa IRQ, safe after boot (avoid for flashing) |
| DIO1      | GPIO16       | D0      | Optional LoRa IRQ (cannot be normal interrupt) |

| DS3231 Pin | ESP-12E GPIO | D Label | Notes                                         |
| ---------- | ------------ | ------- | --------------------------------------------- |
| VCC        | 3.3V         | 3V3     | Some modules allow 5V; 3.3V safe for ESP      |
| GND        | GND          | GND     | Common ground                                 |
| SDA        | GPIO4        | D2      | Boot-safe I²C data                            |
| SCL        | GPIO5        | D1      | Boot-safe I²C clock                           |
| INT / SQW  | GPIO16       | D0      | Optional alarm → can wake ESP from deep sleep |




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
