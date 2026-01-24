#include <Arduino.h>
#include <LoRa.h>
#include "radio.h"

// ================== NODE SETUP ==================
String generateUniqueId() {
    // Compact unique ID using millis() in HEX and a small random hex tail
    char buf[16];
    // millis() -> hex
    sprintf(buf, "%lX", millis());
    String head = String(buf);
    // random tail (4 hex digits)
    int tail = random(0, 0x10000); // 0 .. 0xFFFF
    char tailBuf[8];
    sprintf(tailBuf, "%X", tail);
    String tailStr = String(tailBuf);
    return head + "_" + tailStr;
}

LoRaNode node(generateUniqueId(), 7);

// ================== COLOR LOG MACROS ==================
#define INFO(x)  Serial.println("[INFO] " + String(x))
#define WARN(x)  Serial.println("[WARN] " + String(x))
#define DBG(x)   Serial.println("[DBG]  " + String(x))
#define ERR(x)   Serial.println("[ERR]  " + String(x))

// ================== ISR FLAGS ==================
volatile bool hasLoRaPacket = false;
volatile int lastPacketSize = 0;

void IRAM_ATTR onLoRaEvent(int packetSize) {
    lastPacketSize = packetSize;
    hasLoRaPacket = true;
}

// ================== SERIAL → PACKET PARSER ==================
Packet parseSerialPacket(String packet) {
    // EXPECTED INPUT
    // CHANNEL_ID||MESSAGE_ID||SENDER_ID||MESSAGE||TIMESTAMP

    Packet result;
    result.valid = false;

    String parts[5];
    int index = 0;

    while (packet.length() > 0 && index < 4) {
        int sepIndex = packet.indexOf("||");
        if (sepIndex == -1) {
            parts[index++] = packet;
            break;
        } else {
            parts[index++] = packet.substring(0, sepIndex);
            packet = packet.substring(sepIndex + 2);
        }
    }
    if (index < 4) return result;

    result.channel_id = parts[0];
    result.message_id = parts[1];
    result.sender_id  = parts[2];
    result.message    = parts[3];
    result.time_stamp = generateUniqueId();
    result.valid      = true;

    return result;
}

// ================== SETUP ==================
void setup() {
    Serial.begin(115200);
    while (!Serial) {}

    delay(1000);  // Wait for serial stabilization and other ESP32 to boot

    INFO("=== Initializing LoRa Node ===");

    // ========== ESP32 SYNCHRONIZATION ==========
    // Wait for "ready" message from the other ESP32 before proceeding
    INFO("Waiting for sync signal from other ESP32...");
    bool synced = false;
    
    while (!synced) {
        if (Serial.available()) {
            String syncMsg = Serial.readStringUntil('\n');
            syncMsg.trim();
            
            if (syncMsg == "READY") {
                INFO("Sync signal received! Proceeding with initialization...");
                synced = true;
                break;
            }
        }
        delay(100);  // Small delay to prevent hogging CPU
    }
    // ========== END SYNCHRONIZATION ==========


    INFO("=== Initializing LoRa Node ===");

    if (!node.begin()) {
        ERR("LoRa init failed!");
        while (1);
    }

    INFO("LoRa init success.");
    LoRa.onReceive(onLoRaEvent);
    LoRa.receive();
    INFO("Node Address: " + node.getAddress());
    INFO("Waiting for packets...");
}

// ================== MAIN LOOP ==================
void loop() {

    // ------------------ SERIAL → LORA ------------------
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) return;
        if (line == "RESET") {
            INFO("Reset request received. Restarting...");
            ESP.restart();
        }
        // Ignore debug/system lines
        if (line.startsWith("[D]") || line.startsWith("[LoRa") ||
            line.startsWith("[INFO") || line.startsWith("[WARN") ||
            line.startsWith("[DBG") || line.startsWith("[ERR") ||
            line.startsWith("[FATAL")) return;

        // Parse incoming packet
        Packet pkt = parseSerialPacket(line);
        if (!pkt.valid) {
            WARN("Invalid packet format");
            return;
        }

        node.sendMessage(pkt);
        INFO("TX " + pkt.channel_id + " → " + pkt.message);
    }

    // ------------------ LORA → SERIAL ------------------
    if (hasLoRaPacket) {
        hasLoRaPacket = false;
        node.processReceived(lastPacketSize);
        LoRa.receive();  // Resume listening
    }

    // ------------------ CLEANUP ------------------
    static unsigned long lastCleanup = 0;
    if (millis() - lastCleanup > 30000) {
        lastCleanup = millis();
        node.cleanupSeenMessages();
    }
}
