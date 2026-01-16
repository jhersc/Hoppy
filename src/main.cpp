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
    // EXPEDTED INPUT
    // CHANNEL_ID||MESSAGE_ID||SENDER_ID||MESSAGE||TIMESTAMP

    Packet result;
    result.valid = false;

    String parts[5];
    int index = 0;

    while (packet.length() > 0 && index < 5) {
        int sepIndex = packet.indexOf("||");
        if (sepIndex == -1) {
            parts[index++] = packet;
            break;
        } else {
            parts[index++] = packet.substring(0, sepIndex);
            packet = packet.substring(sepIndex + 2);
        }
    }
    if (index < 5) return result;

    result.channel_id = parts[0];
    result.message_id = parts[1];
    result.sender_id  = parts[2];
    result.message    = parts[3];
    result.time_stamp = parts[4];
    result.valid      = true;

    return result;
}

// ================== SETUP ==================
void setup() {
    Serial.begin(115200);
    while (!Serial) {}

    INFO("=== Initializing LoRa Node ===");

    if (!node.begin()) {
        ERR("LoRa init failed!");
        while (1);
    }

    INFO("LoRa init success.");
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
        LoRa.receive();
    }
}
