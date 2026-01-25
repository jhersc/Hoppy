# Hoppy - ESP32 LoRa Mesh Network

A dual-ESP32 mesh networking project using LoRa (RA-02 / SX1278) modules for long-range wireless communication with flooding-based message propagation.

---

## Overview

**Hoppy** implements a simple mesh network on two ESP32 microcontrollers communicating via:
- **LoRa** (long-range RF) - wireless inter-node communication
- **Serial UART** - inter-MCU synchronization on the same device

### Key Features

✅ **ISR-based LoRa reception** - Interrupt-driven packet handling with `LoRa.onReceive()` callback  
✅ **Message deduplication** - Prevents recursive flooding using message ID tracking  
✅ **Self-echo prevention** - Ignores own transmitted messages  
✅ **Serial synchronization** - Coordinates packet handling between the two ESP32s  
✅ **Automatic timeout cleanup** - Maintains memory efficiency with periodic deduplication table cleanup  

---

## Hardware Requirements

### Components
- 2× **ESP32 (30-pin)** microcontrollers
- 2× **RA-02 LoRa modules** (SX1278, 433 MHz)
- 2× **3.3V regulators** (≥200 mA capacity)
- Breadboard, jumper wires, USB cables

### LoRa Module Pin Functions

| RA-02 Pin | Function | Purpose |
|-----------|----------|---------|
| VCC       | Power    | 3.3V (120-140 mA) |
| GND       | Ground   | Common ground |
| NSS/CS    | SPI      | Chip select |
| SCLK      | SPI      | Clock |
| MOSI      | SPI      | Data out (MCU→Module) |
| MISO      | SPI      | Data in (Module→MCU) |
| RESET     | Control  | Module reset (active low) |
| DIO0      | Interrupt| RxDone/TxDone signal |

---

## Wiring Configuration - ESP32

### SPI Pin Mapping

| RA-02 Pin | ESP32 GPIO | Notes |
|-----------|------------|-------|
| VCC       | 3.3V       | Stable supply required |
| GND       | GND        | Common ground |
| NSS (CS)  | GPIO23     | SPI chip select |
| SCLK      | GPIO13     | SPI clock |
| MOSI      | GPIO19     | SPI MOSI |
| MISO      | GPIO18     | SPI MISO |
| RESET     | GPIO33     | Active-low reset |
| DIO0      | GPIO32     | Interrupt input (RISING edge) |

### Wiring Checklist

- [ ] RA-02 VCC → 3.3V (via regulator)
- [ ] RA-02 GND → ESP32 GND (common reference)
- [ ] RA-02 NSS → GPIO23
- [ ] RA-02 SCLK → GPIO13
- [ ] RA-02 MOSI → GPIO19
- [ ] RA-02 MISO → GPIO18
- [ ] RA-02 RESET → GPIO33
- [ ] RA-02 DIO0 → GPIO32

---

## Software Architecture

### Core Components

#### `radio.h` / `radio.cpp` - LoRa Radio Class
Manages all LoRa communication:
- **`sendMessage()`** - Transmits packet with automatic sender_id stamping
- **`processReceived()`** - Handles incoming LoRa packets
- **`alreadySeen()`** - Checks deduplication table
- **`markAsSeen()`** - Records message for future filtering
- **`recentlySent()`** - Prevents immediate self-echo
- **`cleanupSeenMessages()`** - Periodic memory management

#### `main.cpp` - Main Application Logic
Coordinates between LoRa and Serial:
- ISR callback: `onLoRaEvent()` - Triggered on DIO0 (packet received)
- Serial input handling - Parses and forwards messages
- Packet flooding - Rebroadcasts non-duplicate, non-self messages

### Packet Structure

```
CHANNEL_ID||MESSAGE_ID||SENDER_ID||MESSAGE||TIMESTAMP
```

Example:
```
CH001||MSG123||NODE_02||HelloWorld||1A2B3C4D
```

---

## Setup & Build

### Prerequisites

- **PlatformIO** installed on VS Code
- **Arduino IDE** or compatible (optional)
- USB drivers for ESP32

### Installation

1. **Clone/download this repository**
   ```bash
   cd ESP32/Hoppy
   ```

2. **Configure Serial Ports** (platformio.ini)
   ```ini
   [env:esp32-hoppy]
   upload_port = /dev/ttyUSB0    ; or COM3, COM4 on Windows
   monitor_port = /dev/ttyUSB0
   monitor_speed = 115200
   ```

3. **Build**
   ```bash
   platformio run
   ```

4. **Upload to first ESP32**
   ```bash
   platformio run --target upload --upload-port /dev/ttyUSB0
   ```

5. **Upload to second ESP32** (change port)
   ```bash
   platformio run --target upload --upload-port /dev/ttyUSB1
   ```

6. **Monitor output**
   ```bash
   platformio device monitor --port /dev/ttyUSB0
   ```

---

## Testing & Verification

### Basic Sanity Check

Both ESP32s should boot and log initialization:
```
[INFO] === Initializing LoRa Node ===
[INFO] LoRa init success.
[INFO] Node Address: 1A2B3C4D_E5F6
[INFO] Waiting for packets...
```

### Send a Test Message

From the serial terminal (send raw packet):
```
CH001||MSG001||NODE_01||Hello||1A2B3C4D
```

**Expected behavior:**
- Node 1 (sender) → broadcasts via LoRa
- Node 2 (receiver) → logs `[DBG] RX: CH001||MSG001||...`
- Node 2 → rebroadcasts to ensure delivery
- Node 1 → receives echo, logs `[DBG] RX (echo)` (ignored)

### Deduplication Test

Send the same message twice with identical MESSAGE_ID:
```
CH001||MSG001||NODE_01||Hello||1A2B3C4D
CH001||MSG001||NODE_01||Hello||1A2B3C4D
```

**Expected:** Second packet logged as `[DBG] RX (dup): ...` and NOT rebroadcasted

---

## Message Flow Diagram

```
┌─────────────┐                            ┌─────────────┐
│   ESP32 #1  │                            │   ESP32 #2  │
├─────────────┤                            ├─────────────┤
│             │────LoRa────────────────→  │ DIO0 IRQ    │
│   Loop()    │  [packet received]         │   ISR set   │
│             │                            │  flag = 1   │
│             │                            │             │
│             │  ←──Serial UART──────────  │ Serial out  │
│ Serial in   │  [forwarded packet]        │ processRX() │
│ parseSerialPacket()                      │             │
│ markAsSeen(id)  ✓ PREVENT ECHO           │ alreadySeen │
│ sendMessage()                            │ markAsSeen  │
│   LoRa.TX   │                            │             │
│   Mark sent │────LoRa─────────────────→ │ [received]  │
│   sentMessages                           │ RX (echo)   │
│             │                            │ [ignored]   │
└─────────────┘                            └─────────────┘
```

---

## Troubleshooting

### Issue: "LoRa init failed!"

**Cause:** SPI pins misconfigured or module not powered  
**Solution:** 
- Verify wiring against [Wiring Configuration](#wiring-configuration---esp32)
- Check 3.3V regulator capacity (≥200 mA)
- Measure voltage: should be stable 3.3V under load

### Issue: Recursive Message Flooding

**Cause:** Deduplication not working, or messages sent to LoRa twice  
**Solution:**
- Verify `markAsSeen()` called in `processReceived()`
- Check that `alreadySeen()` is checked BEFORE rebroadcasting
- Ensure `sentMessages` timeout is configured correctly

### Issue: Serial Output Gibberish

**Cause:** Wrong baud rate  
**Solution:** Confirm both:
- `Serial.begin(115200)` in code
- Terminal set to 115200 baud

### Issue: No LoRa Reception

**Cause:** DIO0 interrupt not firing  
**Solution:**
- Verify GPIO32 wired to RA-02 DIO0
- Check DIO0 pulled HIGH (not floating)
- Confirm `LoRa.onReceive(onLoRaEvent)` registered
- Verify `IRAM_ATTR` in ISR callback

---

## Performance Characteristics

| Metric | Value |
|--------|-------|
| Max LoRa Packet Size | 255 bytes |
| Deduplication Timeout | 60 seconds |
| Self-echo Prevention Timeout | 2 seconds |
| Cleanup Interval | 30 seconds |
| Spreading Factor | 7 (default) |
| Frequency | 433 MHz |
| Baud Rate | 115200 |

---

## Future Improvements

- [ ] AODV routing protocol integration
- [ ] Configurable message TTL (time-to-live)
- [ ] Packet priority queue
- [ ] Acknowledge mechanism (ACK)
- [ ] Encryption (AES-128)
- [ ] Web dashboard for network monitoring

---

## File Structure

```
ESP32/Hoppy/
├── src/
│   ├── main.cpp          # Main application logic
│   ├── radio.h           # LoRaNode class header
│   ├── radio.cpp         # LoRa implementation
│   ├── preferencesHandler.h
│   ├── sender.txt        # Reference packet format
│   └── receiver.txt      # Expected packet format
├── lib/
│   └── README
├── test/
│   └── README
├── platformio.ini        # PlatformIO configuration
└── README.md             # This file
```

---

## License

See [LICENSE](LICENSE) file for details.

---

## Author & Contact

**Project:** Hoppy - ESP32 LoRa Mesh Network  
**Date Created:** January 2026  
**Status:** Active Development
