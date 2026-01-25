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
        WARN("LoRa init failed - check wiring/pins");
        DBG("SPI pins - SCK:" + String(pin_sck) + " MISO:" + String(pin_miso) + 
            " MOSI:" + String(pin_mosi) + " SS:" + String(pin_ss));
        DBG("RST pin: " + String(pin_rst) + " DIO0 pin: " + String(pin_dio0));
        return false;
    }
    
    LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);
    LoRa.setSpreadingFactor(sf);
    LoRa.setSyncWord(0x34);  // Set sync word (LoRaWAN)
    INFO("LoRa initialized at " + String(frequency) + "Hz, SF=" + String(sf));
    return true;
}

// ================== SEND ==================
void LoRaNode::sendMessage(const Packet &pkt) {
    // Create packet with our address as sender
    Packet outgoing = pkt;
    outgoing.sender_id = address;
    
    String raw =
        outgoing.channel_id + "||" +
        outgoing.message_id + "||" +
        outgoing.sender_id  + "||" +
        outgoing.message    + "||" +
        outgoing.time_stamp;

    LoRa.beginPacket();
    LoRa.print(raw);
    LoRa.endPacket();

    DBG("TX: " + raw);
    Serial.println(raw); // sends to other MCU
    
    // Mark this message as sent to avoid immediate self-echo
    markMessageSent(outgoing.message_id);
    
    LoRa.receive();
}

// ================== RECEIVE ==================
void LoRaNode::processReceived(int packetSize) {
    if (packetSize <= 0) return;

    String raw;
    while (LoRa.available()) raw += (char)LoRa.read();

    parseRawPacket(raw, received_packet);
    if (!received_packet.valid) return;
    
    // Ignore messages from ourselves
    if (received_packet.sender_id == address) {
        DBG("RX (self): " + raw);
        return;
    }
    
    // Avoid immediate self-echo (received our own transmission)
    if (recentlySent(received_packet.message_id)) {
        DBG("RX (echo): " + raw);
        return;
    }
    
    // Flood control: ignore duplicates
    if (alreadySeen(received_packet.message_id)) {
        DBG("RX (dup): " + raw);
        return;
    }

    DBG("RX: " + raw);
    
    // Mark as seen before rebroadcasting
    markAsSeen(received_packet.message_id);
    
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
    return seenMessages.count(msgId) > 0;
}

void LoRaNode::markAsSeen(const String &msgId) {
    seenMessages[msgId] = millis();
}

bool LoRaNode::recentlySent(const String &msgId) {
    if (sentMessages.count(msgId)) return true;
    return false;
}

void LoRaNode::markMessageSent(const String &msgId) {
    sentMessages[msgId] = millis();
}

void LoRaNode::cleanupSeenMessages() {
    unsigned long now = millis();
    for (auto it = seenMessages.begin(); it != seenMessages.end();) {
        if (now - it->second > SEEN_TIMEOUT)
            it = seenMessages.erase(it);
        else ++it;
    }
    
    // Also cleanup recently sent messages
    for (auto it = sentMessages.begin(); it != sentMessages.end();) {
        if (now - it->second > SENT_TIMEOUT)
            it = sentMessages.erase(it);
        else ++it;
    }
}
