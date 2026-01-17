#include "radio.h"

// ================== LOG HELPERS ==================
#define INFO(x) Serial.println("[INFO] " + String(x))
#define WARN(x) Serial.println("[WARN] " + String(x))
#define DBG(x)  Serial.println("[DBG]  " + String(x))

// ================== CONSTRUCTOR ==================
LoRaNode::LoRaNode(String nodeAddress, int spreadingFactor,
                   int sck, int miso, int mosi, int ss,
                   int rst, int dio0)
    : address(nodeAddress), sf(spreadingFactor),
      pin_sck(sck), pin_miso(miso), pin_mosi(mosi),
      pin_ss(ss), pin_rst(rst), pin_dio0(dio0) {}

// ================== INIT ==================
bool LoRaNode::begin(long frequency) {
    SPI.begin(pin_sck, pin_miso, pin_mosi, pin_ss);
    LoRa.setPins(pin_ss, pin_rst, pin_dio0);

    if (!LoRa.begin(frequency)) {
        WARN("LoRa init failed");
        return false;
    }

    LoRa.setSpreadingFactor(sf);
    INFO("LoRa initialized");
    return true;
}

// ================== SEND ==================
void LoRaNode::sendMessage(const Packet &pkt) {
    String raw =
        pkt.channel_id + "||" +
        pkt.message_id + "||" +
        pkt.sender_id  + "||" +
        pkt.message    + "||" +
        pkt.time_stamp;

    LoRa.beginPacket();
    LoRa.print(raw);
    LoRa.endPacket();

    DBG("TX: " + raw);
    LoRa.receive();
}

// ================== RECEIVE ==================
void LoRaNode::processReceived(int packetSize) {
    if (packetSize <= 0) return;

    String raw;
    while (LoRa.available()) raw += (char)LoRa.read();

    parseRawPacket(raw, received_packet);
    if (!received_packet.valid) return;
    if (received_packet.sender_id == address) return;
    
    // Flood control: ignore duplicates
    if (alreadySeen(received_packet.message_id)) return;

    DBG("RX: " + raw);
    
    // Rebroadcast (flood)
    sendMessage(received_packet);
}

// ================== PARSERS ==================
void LoRaNode::parseRawPacket(const String &raw, Packet &pkt) {
    pkt.valid = false;

    int i1 = raw.indexOf("||");
    int i2 = raw.indexOf("||", i1 + 2);
    int i3 = raw.indexOf("||", i2 + 2);
    int i4 = raw.indexOf("||", i3 + 2);

    if (i1 < 0 || i2 < 0 || i3 < 0 || i4 < 0) return;

    pkt.channel_id = raw.substring(0, i1);
    pkt.message_id = raw.substring(i1 + 2, i2);
    pkt.sender_id  = raw.substring(i2 + 2, i3);
    pkt.message    = raw.substring(i3 + 2, i4);
    pkt.time_stamp = raw.substring(i4 + 2);
    pkt.valid = true;
}

bool LoRaNode::alreadySeen(const String &msgId) {
    if (seenMessages.count(msgId)) return true;
    
    seenMessages[msgId] = millis();
    return false;
}

void LoRaNode::cleanupSeenMessages() {
    unsigned long now = millis();
    for (auto it = seenMessages.begin(); it != seenMessages.end();) {
        if (now - it->second > SEEN_TIMEOUT)
            it = seenMessages.erase(it);
        else ++it;
    }
}
