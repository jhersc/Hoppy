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


### ** 30 Pins Mode**
| Peripheral                     | Signal   | ESP32 GPIO | Notes           |
| ------------------------------ | -------- | ---------- | --------------- |
| **TFT Display (ILI9341, SPI)** | MOSI     | **23**     | Shared SPI      |
|                                | MISO     | **19**     | Shared SPI      |
|                                | SCLK     | **18**     | Shared SPI      |
|                                | CS       | **5**      | TFT chip select |
|                                | DC       | **2**      | Data / Command  |
|                                | RST      | **EN**     | ESP32 reset     |
|                                | VCC      | 3.3V       | Power           |
|                                | GND      | GND        | Ground          |
| **4×5 Keypad**                 | Row 1    | **13**     | Output          |
|                                | Row 2    | **12**     | Output          |
|                                | Row 3    | **14**     | Output          |
|                                | Row 4    | **27**     | Output          |
|                                | Row 5    | **26**     | Output          |
|                                | Column 1 | **25**     | Input           |
|                                | Column 2 | **33**     | Input           |
|                                | Column 3 | **32**     | Input           |
|                                | Column 4 | **35**     | Input-only      |
| **RA-02 LoRa (SX1278, SPI)**   | SCK      | **18**     | Shared SPI      |
|                                | MOSI     | **23**     | Shared SPI      |
|                                | MISO     | **19**     | Shared SPI      |
|                                | NSS (CS) | **16**     | LoRa CS         |
|                                | RST      | **17**     | LoRa reset      |
|                                | DIO0     | **39**     | Input-only IRQ  |
|                                | VCC      | 3.3V       | ⚠️ Never 5V     |
|                                | GND      | GND        | Ground          |
| **RTC (DS3231 / DS1307)**      | SDA      | **21**     | I²C data        |
|                                | SCL      | **22**     | I²C clock       |
|                                | VCC      | 3.3V       | Power           |
|                                | GND      | GND        | Ground          |
| **USB / Debug**                | TX0      | **1**      | USB serial      |
|                                | RX0      | **3**      | USB serial      |

