#include <Arduino.h>
#include <LoRa.h>
#include "radio.h"

// ================== NODE SETUP ==================
LoRaNode node("02", 7);

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

// ================== SERIAL PARSER ==================
ParsedPacket parseSerialPacket(String line) {
    ParsedPacket pkt;
    pkt.valid = false;
    line.trim();
    if (line.length() == 0) return pkt;

    String parts[8];
    int index = 0;
    while (line.length() > 0 && index < 8) {
        int sepIndex = line.indexOf("||");
        if (sepIndex == -1) {
            parts[index++] = line;
            break;
        } else {
            parts[index++] = line.substring(0, sepIndex);
            line = line.substring(sepIndex + 2);
        }
    }

    if (index < 8) return pkt;

    pkt.timestamp_hex = parts[0];
    pkt.channel_name  = parts[1];
    pkt.channel_id    = parts[2];
    pkt.sender        = parts[3];
    pkt.message_id    = parts[4];
    pkt.length        = parts[5].toInt();
    pkt.is_channel    = (parts[6].toInt() == 1);
    pkt.message       = parts[7];
    pkt.valid         = true;
    return pkt;
}

// ================== SETUP ==================
unsigned long lastHeartbeat = 0;

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

        if (line.startsWith("[") && line.indexOf("]") != -1) return;

        ParsedPacket pkt = parseSerialPacket(line);
        if (!pkt.valid) {
            WARN("Invalid serial packet discarded.");
            return;
        }

        if (pkt.channel_name == "DATA" && pkt.channel_id.length() > 0) {
            node.sendDataAODV(pkt.channel_id, pkt.message);
            INFO("AODV TX: " + pkt.message);
        } else {
            node.sendMessage(pkt);
            DBG("Raw TX: " + line);
        }
    }


    // ------------------ LORA → SERIAL ------------------
    if (hasLoRaPacket) {
        hasLoRaPacket = false;

        node.processReceived(lastPacketSize);
        ParsedPacket pkt = node.getLastReceivedPacket();

        if (pkt.valid) {
            String out = pkt.timestamp_hex + "||" +
                         pkt.channel_name + "||" +
                         pkt.channel_id + "||" +
                         pkt.sender + "||" +
                         pkt.message_id + "||" +
                         String(pkt.length) + "||" +
                         String(pkt.is_channel ? 1 : 0) + "||" +
                         pkt.message;
            Serial.println(out);

            if (pkt.channel_name == "RREQ" || pkt.channel_name == "RREP") {
                node.receiveAODV(pkt);
            }
        }

        node.clearLastReceivedPacket();
        LoRa.receive();
    }

    // ------------------ HEARTBEAT ------------------
    if (millis() - lastHeartbeat > 5000) {
        lastHeartbeat = millis();
        DBG("Listening...");
    }
}
