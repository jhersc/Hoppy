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
void LoRaNode::sendToLoRa(const Packet &pkt) {
    // Create packet with our address as sender
    // Packet format: date_and_time||message_id||channel_id||
    // channel_name||sender_name||sender_id||content
    Packet outgoing = pkt;
    outgoing.sender_id = address;
    
    String raw =
        outgoing.date_and_time + "||" +
        outgoing.message_id    + "||" +
        outgoing.channel_id    + "||" +
        outgoing.channel_name  + "||" +
        outgoing.sender_name   + "||" +
        outgoing.sender_id     + "||" +
        outgoing.content;
        
    LoRa.beginPacket();
    LoRa.print(raw);
    LoRa.endPacket();

    LoRa.receive();

    DBG("LORA_TX: " + raw);
    outgoing.time_stamp = millis();
    // if this is ours, then we sent it
    if (outgoing.sender_id == address) {
        sentMessages[outgoing.message_id] = outgoing.time_stamp;
        return;
    }
    // if this is theirs, then we mark it as seen
    seenMessages[outgoing.message_id] = outgoing.time_stamp;
}
void LoRaNode::sendUart(const Packet &pkt) {
    Packet outgoing = pkt;Packet outgoing = pkt;
    outgoing.sender_id = address;
    

    String raw =
                              "msg||" +
        outgoing.date_and_time + "||" +
        outgoing.message_id    + "||" +
        outgoing.channel_id    + "||" +
        outgoing.channel_name  + "||" +
        outgoing.sender_name   + "||" +
        outgoing.sender_id     + "||" +
        outgoing.content;

    DBG("UART_TX: " + raw);
    Serial.println(raw); // sends to other MCU
}

void LoRaNode::sendUartUpdate(const Packet &pkt) {
    // Create packet with our address as sender
    Packet outgoing = pkt;
    outgoing.sender_id = address;
    
    String raw =
        "ack||" +
        outgoing.message_id    + "||" +
        String(outgoing.rssi ? outgoing.rssi : -1) + "||" +
        String(outgoing.snr ? outgoing.snr : -1)   + "||" +
        String(outgoing.latency ? outgoing.latency : -1);

    DBG("UART_TX: " + raw);
    Serial.println(raw); // sends to other MCU
    seenMessages[outgoing.message_id] = millis();
     
}
/**
 * Process a received LoRa Packet
 * Sends it to LoRa and UART as needed
 */
void LoRaNode::processReceived(int packetSize) {
    if (packetSize <= 0) return;

    String raw;
    while (LoRa.available()) raw += (char)LoRa.read();
    received_packet.snr = LoRa.packetSnr();
    received_packet.rssi = LoRa.packetRssi();  
    parseRawPacket(raw, received_packet);
    if (!received_packet.valid) return;
    
    // Calculate latency only for our OWN messages (when we receive our own transmission back)
    if (received_packet.sender_id == address) {

        if (sentMessages.count(received_packet.message_id)) {
            unsigned long sent_time = sentMessages[received_packet.message_id];
            received_packet.latency = millis() - sent_time;
            DBG("RX: Latency for our message " + received_packet.message_id + " = " + String(received_packet.latency) + "ms");

            // Send ACK via UART
            // Format: ack||message_id||rssi||snr||latency
            String lat = "ack||" +
                received_packet.message_id + "||" +
                String(received_packet.rssi) + "||" +
                String(received_packet.snr) + "||" +
                String(received_packet.latency);

            Serial.println(lat);
        }
        // DBG("RX (self): " + raw);
        return;
    }
    
    // For neighbor messages, latency remains as received (or 0 if not set)
    // received_packet.latency = 0;
    
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

    // Track receive count
    received_packet.receive_count = ++messageReceiveCount[received_packet.message_id];

    DBG("RX: " + raw);
    
    // Mark as seen before rebroadcasting
    markAsSeen(received_packet.message_id);
    
    // Rebroadcast (flood) if receive_count <= MAX_RECEIVE_COUNT
    if (received_packet.receive_count <= MAX_RECEIVE_COUNT) {
        sendToLoRa(received_packet);
    } else {
        DBG("RX: Skipping rebroadcast - max receive count reached for " + received_packet.message_id);
    }
}

// ================== PARSERS ==================
void LoRaNode::parseRawPacket(const String &raw, Packet &pkt) {
    pkt.valid = false;
    pkt.latency = 0;
    pkt.receive_count = 0;
    // ack||message_id||rssi||snr||latency
    if (raw.substring(0, 5) == "ack||") {
        // ACK packet
        String r = raw.substring(5);
        int i1 = r.indexOf("||");
        int i2 = r.indexOf("||", i1 + 2);
        int i3 = r.indexOf("||", i2 + 2);
        if (i1 < 0 || i2 < 0 || i3 < 0) return;
        
        pkt.message_id = r.substring(0, i1); 
        pkt.rssi = r.substring(i1 + 2, i2).toInt();
        pkt.snr  = r.substring(i2 + 2, i3).toFloat();
        pkt.latency = r.substring(i3 + 2).toInt();
        pkt.valid = true;
        return;
    }
    /**
     * msg||date_and_time||message_id||sender_id||
     * channel_id||sender_name||channel_name||content
     *  */ 
    String r = raw.substring(5);
    int i1 = r.indexOf("||");
    int i2 = r.indexOf("||", i1 + 2);
    int i3 = r.indexOf("||", i2 + 2);
    int i4 = r.indexOf("||", i3 + 2);
    int i5 = r.indexOf("||", i4 + 2);
    int i6 = r.indexOf("||", i5 + 2);
    // int i7 = raw.indexOf("||", i6 + 2);

    if (i1 < 0 || i2 < 0 || i3 < 0 || i4 < 0 || i5 < 0 || i6 < 0) return;

    pkt.date_and_time = r.substring(0, i1);
    pkt.message_id    = r.substring(i1 + 2, i2);
    pkt.sender_id     = r.substring(i2 + 2, i3);
    pkt.channel_id    = r.substring(i3 + 2, i4);
    pkt.sender_name   = r.substring(i4 + 2);
    pkt.channel_name  = r.substring(i5 + 2, i6);   
    pkt.content       = r.substring(i6 + 2);

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
    
    // Cleanup sent timestamps for latency calculation
    for (auto it = sentTimestamps.begin(); it != sentTimestamps.end();) {
        if (now - it->second > LATENCY_TIMEOUT)
            it = sentTimestamps.erase(it);
        else ++it;
    }
    
    // Cleanup message receive counts
    for (auto it = messageReceiveCount.begin(); it != messageReceiveCount.end();) {
        // We can clean these up safely since they're time-based anyway
        // Using same timeout as seen messages
        if (seenMessages.count(it->first) == 0) {
            it = messageReceiveCount.erase(it);
        } else {
            ++it;
        }
    }
}
