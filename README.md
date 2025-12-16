


# ESP-12E (ESP8266) ↔ RA-02 (SX1278) LoRa Wiring Guide

This guide provides a safe and stable wiring configuration to connect the ESP-12E (30-pin module) to the RA-02 (SX1278) LoRa module.

---

## Pin Connection Table

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
| DIO2      | —            | —               | Not required (optional) |

---

## Important Notes

- **Do NOT use GPIO0 or GPIO2** for RA-02 signals (boot-sensitive pins).  
- GPIO16 **cannot generate interrupts**, so avoid using it for DIO0.  
- Add **10 µF + 0.1 µF capacitors** near the RA-02 VCC to stabilize power.  
- Ensure a **stable 3.3 V regulator** capable of handling peak currents (~120–140 mA) from the RA-02.  

---

## Example Arduino Initialization

```cpp
#include <SPI.h>
#include <LoRa.h>

void setup() {
  SPI.begin(14, 12, 13, 15); // SCK, MISO, MOSI, NSS
  LoRa.setPins(15, 16, 5);   // NSS, RESET, DIO0

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed!");
    while (1);
  }
  Serial.println("LoRa init OK!");
}

void loop() {
  // Your LoRa code here
}



# ESP32 ↔ RA-02 (SX1278) LoRa Wiring Guide

This guide provides a safe wiring configuration to connect an ESP32 (30-pin module) to the RA-02 (SX1278) LoRa module using SPI.

---

## Pin Connection Table

| RA-02 Pin | ESP32 GPIO | Notes |
|-----------|------------|-------|
| VCC       | 3.3V       | **3.3 V only**, ≥120 mA |
| GND       | GND        | Common ground |
| NSS (CS)  | GPIO23     | SPI chip select |
| SCK       | GPIO13     | SPI clock |
| MOSI      | GPIO19     | SPI MOSI |
| MISO      | GPIO18     | SPI MISO |
| RESET     | GPIO33     | Output only (module reset) |
| DIO0      | GPIO32     | Interrupt (RX/TX done) |
| DIO1      | —          | Optional |
| DIO2      | —          | Optional |

---

## Important Notes

- Ensure the **3.3 V supply** can handle peak currents (~120–140 mA) during LoRa transmission.  
- Add **10 µF + 0.1 µF capacitors** near the RA-02 VCC for voltage stability.  
- All listed GPIOs are **safe for SPI and interrupts** on the ESP32.  

---

## Example Arduino Initialization

```cpp
#include <SPI.h>
#include <LoRa.h>

#define SCK 13
#define MISO 18
#define MOSI 19
#define SS 23
#define RST 33
#define DIO0 32

void setup() {
  SPI.begin(SCK, MISO, MOSI, SS);  // Initialize SPI
  LoRa.setPins(SS, RST, DIO0);     // Set LoRa pins

  if (!LoRa.begin(433E6)) {        // Change frequency as needed
    Serial.println("LoRa init failed!");
    while (1);
  }
  Serial.println("LoRa init OK!");
}

void loop() {
  // Your LoRa code here
}

