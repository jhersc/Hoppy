#include <Arduino.h>
#include <LoRa.h>
#include "radio.h" // LoRaNode class header

// ================== NODE SETUP ==================
// Initialize a LoRa node with address "02" and spreading factor 7
LoRaNode node("02", 7);

// ================== ISR FLAGS ==================
volatile bool hasLoRaPacket = false;
volatile int lastPacketSize = 0;

// ================== ISR ==================
void IRAM_ATTR onLoRaEvent(int packetSize) {
    lastPacketSize = packetSize;
    hasLoRaPacket = true;
}

// ================== SERIAL PARSER ==================
ParsedPacket parsePacket(String packet) {
    ParsedPacket result;
    result.valid = false;
    packet.trim();
    if (packet.length() == 0) return result;

    String parts[8];
    int index = 0;

    while (packet.length() > 0 && index < 8) {
        int sepIndex = packet.indexOf("||");
        if (sepIndex == -1) {
            parts[index++] = packet;
            break;
        } else {
            parts[index++] = packet.substring(0, sepIndex);
            packet = packet.substring(sepIndex + 2);
        }
    }

    if (index < 8) return result;

    result.timestamp_hex = parts[0];
    result.channel_name   = parts[1];
    result.channel_id     = parts[2];
    result.sender         = parts[3];
    result.message_id     = parts[4];
    result.length         = parts[5].toInt();
    result.is_channel     = (parts[6].toInt() == 1);
    result.message        = parts[7];
    result.valid          = true;
    return result;
}

// ================== SETUP ==================
unsigned long lastHeartbeat = 0;

void setup() {
    Serial.begin(115200);
    while (!Serial) {}

    Serial.println("[D] === Initializing LoRa Node ===");

    if (!node.begin()) {
        Serial.println("[FATAL] LoRa init failed!");
        while (1);
    }

    Serial.println("[INFO] LoRa init success.");

    LoRa.onReceive(onLoRaEvent);
    LoRa.receive();
}

// ================== MAIN LOOP ==================
void loop() {
        // ------------------ SERIAL → LORA ------------------
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) return;

        // Skip debug/system lines
        if (line.startsWith("[") && line.indexOf("]") != -1) return;

        // Parse the serial line into a ParsedPacket
        ParsedPacket pkt = parsePacket(line);
        if (!pkt.valid) {
            Serial.println("[D] [Parser] Invalid serial packet, discarded.");
            return;
        }

        // Send parsed packet via LoRa (uses the LoRaNode::sendMessage(const ParsedPacket &))
        node.sendMessage(pkt);
        Serial.println("[LoRa TX] " + line);
    }

    // ------------------ LORA → SERIAL ------------------
if (hasLoRaPacket) {
    hasLoRaPacket = false;

    node.processReceived(lastPacketSize);
    ParsedPacket pkt = node.getLastReceivedPacket();

    if (pkt.valid) {
        // Convert parsed LoRa packet into the new unified format
        String out = pkt.timestamp_hex + "||" +         // timestamp_hex
                     pkt.channel_name + "||" +          // channel_name (placeholder, if unknown)
                     pkt.channel_id + "||" +         // channel_id (or destination)
                     pkt.sender + "||" +              // sender
                     pkt.message_id + "||" +          // message_id
                     String(pkt.length) + "||" +      // length
                     String(pkt.is_channel ? 1 : 0) + "||" + // is_channel
                     pkt.message;                     // message content

        Serial.println(out); // Echo to Serial for the main MCU to handle
    }

    node.clearLastReceivedPacket();
    LoRa.receive();
}

    // ------------------ HEARTBEAT ------------------
    if (millis() - lastHeartbeat > 5000) {
        lastHeartbeat = millis();
        Serial.println("[LoRa] Listening...");
    }
}
