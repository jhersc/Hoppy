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
#define INFO(x)  Serial.println(String("\033[32m[INFO]\033[0m ") + x)
#define WARN(x)  Serial.println(String("\033[33m[WARN]\033[0m ") + x)
#define DBG(x)   Serial.println(String("\033[36m[DBG]\033[0m ") + x)
#define ERR(x)   Serial.println(String("\033[31m[ERR]\033[0m ")  + x)

// ================== ISR FLAGS ==================
volatile bool hasLoRaPacket = false;
volatile int lastPacketSize = 0;

void IRAM_ATTR onLoRaEvent(int packetSize) {
    lastPacketSize = packetSize;
    hasLoRaPacket = true;
}

// ================== SERIAL → PACKET PARSER ==================
Packet parseSerialPacket(String line) {
    Packet pkt;
    pkt.valid = false;
    line.trim();
    if (line.length() == 0) return pkt;

    // Expected formats:
    // DATA||<dest>||<message>
    // RAW||<payload>

    if (line.startsWith("DATA||")) {
        int p1 = line.indexOf("||", 6);
        if (p1 < 0) return pkt;

        String dest = line.substring(6, p1);
        String msg  = line.substring(p1 + 2);

        pkt.channel_id = "DATA";
        pkt.message_id = String(millis());
        pkt.sender_id  = node.getAddress();
        pkt.message    = msg;
        pkt.valid = true;
        return pkt;
    }

    if (line.startsWith("RAW||")) {
        pkt.channel_id = "RAW";
        pkt.message_id = String(millis());
        pkt.sender_id  = node.getAddress();
        pkt.message    = line.substring(5);
        pkt.valid = true;
        return pkt;
    }

    WARN("Unknown serial format");
    return pkt;
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

        if (line.startsWith("DATA||")) {
            Packet pkt = parseSerialPacket(line);
            if (!pkt.valid) {
                WARN("Invalid DATA packet");
                return;
            }

            node.sendMessage(pkt);
            INFO("TX DATA → " + pkt.message);
        }
        else {
            Packet pkt = parseSerialPacket("RAW||" + line);
            if (!pkt.valid) return;

            node.sendMessage(pkt);
            DBG("TX RAW → " + pkt.message);
        }
    }

    // ------------------ LORA → SERIAL ------------------
    if (hasLoRaPacket) {
        hasLoRaPacket = false;

        node.processReceived(lastPacketSize);
        LoRa.receive();
    }
}
