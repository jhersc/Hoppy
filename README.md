# ESP-12E & ESP32 ↔ RA-02 (SX1278) LoRa Wiring Guide

This guide provides stable wiring configurations to connect either an **ESP-12E (ESP8266, 30-pin)** or an **ESP32 (30-pin)** module to the **RA-02 (SX1278) LoRa module** using SPI.

---

## 1. ESP-12E (ESP8266) Wiring

### Pin Connection Table


| Function         | ESP-12E GPIO | D Label | Notes                            |
| ---------------- | ------------ | ------- | -------------------------------- |
| LoRa CS          | 15           | D8      | Boot-safe                        |
| LoRa SCK         | 14           | D5      | SPI clock                        |
| LoRa MOSI        | 13           | D7      | SPI MOSI                         |
| LoRa MISO        | 12           | D6      | SPI MISO                         |
| LoRa RESET       | 16           | D0      | Optional reset / RTC alarm wake  |
| LoRa DIO0        | 2            | D4      | Safe, supports interrupts        |
| DS3231 SDA       | 4            | D2      | I²C data                         |
| DS3231 SCL       | 5            | D1      | I²C clock                        |
| DS3231 INT / SQW | 16           | D0      | Optional alarm → deep sleep wake |
| Serial TX → MCU  | 1            | D10     | Safe after flashing              |
| Serial RX ← MCU  | 3            | D9      | Safe after flashing              |




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
