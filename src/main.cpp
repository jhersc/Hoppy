#include <Arduino.h>
#include <LoRa.h>
#include "radio.h"
#include "preferencesHandler.h"


// ================== NODE SETUP ==================
LoRaNode node(generateUniqueId(), 8);
std::map <String, unsigned long> sentMessages;
std::map <String, unsigned long> seenMessages;
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
    PreferencesHandlerBegin();
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
        if (line.startsWith("msg||") || line.startsWith("ack||")) line = line.substring(5);
        // Parse incoming packet
        Packet pkt;
        DBG(line);
        parseRawPacket(line, pkt);
        if (!pkt.valid) {
            WARN("Invalid packet format");
            return;
        }

        
        // node.sendToController(pkt);
        node.sendToLoRa(pkt);
        INFO("TX " + pkt.channel_id + " → " + pkt.content);
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
        // node.cleanupSeenMessages();
        // for message in sentMessage.timestamps:
        //      if message.timestamp > 30000:
        //          delete(message_id)
    }
}
