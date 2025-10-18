#include "radio.h"

// ================== CONSTRUCTOR ==================
LoRaNode::LoRaNode(String nodeAddress, int spreadingFactor,
                   int sck, int miso, int mosi, int ss,
                   int rst, int dio0)
    : address(nodeAddress), sf(spreadingFactor), messageInterval(0), sendCounter(0),
      pin_sck(sck), pin_miso(miso), pin_mosi(mosi),
      pin_ss(ss), pin_rst(rst), pin_dio0(dio0) {}

// ================== INITIALIZATION ==================
bool LoRaNode::begin(long frequency) {
    SPI.begin(pin_sck, pin_miso, pin_mosi, pin_ss);
    LoRa.setPins(pin_ss, pin_rst, pin_dio0);

    if (!LoRa.begin(frequency)) {
        Serial.println("[FATAL] LoRa init failed.");
        return false;
    }

    LoRa.setSpreadingFactor(sf);
    Serial.println("[INFO] LoRa initialized successfully.");
    return true;
}

void LoRaNode::setMessageInterval(unsigned long ms) {
    messageInterval = ms;
}

// ================== SEND MESSAGE ==================
void LoRaNode::sendMessage(const ParsedPacket &pkt) {
    sendCounter++;

    String packet =
        pkt.timestamp_hex + "||" +
        pkt.channel_name  + "||" +
        pkt.channel_id    + "||" +
        pkt.sender        + "||" +
        pkt.message_id    + "||" +
        String(pkt.length) + "||" +
        String(pkt.is_channel ? 1 : 0) + "||" +
        pkt.message;

    LoRa.beginPacket();
    LoRa.print(packet);
    LoRa.endPacket();

    Serial.println("[LoRa TX] " + packet);
    LoRa.receive();
}

// ================== RECEIVE MESSAGE ==================
void LoRaNode::processReceived(int packetSize) {
    if (packetSize <= 0) return;

    String raw;
    while (LoRa.available()) raw += (char)LoRa.read();

    if (raw.isEmpty()) {
        Serial.println("[D] Empty LoRa payload received.");
        return;
    }

    Serial.println("[D] RAW RX: " + raw);
    parseRawPacket(raw, received_packet);

    if (!received_packet.valid) return;

    if (received_packet.sender == address) return; // ignore self

    // Automatically handle AODV packets
    if (received_packet.channel_name == "RREQ" || received_packet.channel_name == "RREP") {
        receiveAODV(received_packet);
    }
}

// ================== PARSER ==================
void LoRaNode::parseRawPacket(String raw, ParsedPacket &pkt) {
    pkt.valid = false;
    raw.trim();
    if (raw.length() == 0) return;

    String parts[8];
    int lastIndex = 0;
    int partIndex = 0;

    for (; partIndex < 7; ++partIndex) {
        int sepIndex = raw.indexOf("||", lastIndex);
        if (sepIndex == -1) return;
        parts[partIndex] = raw.substring(lastIndex, sepIndex);
        parts[partIndex].trim();
        lastIndex = sepIndex + 2;
    }
    parts[7] = raw.substring(lastIndex);
    parts[7].trim();

    int parsedLength = 0;
    bool parsedIsChannel = false;

    if (parts[5].length() > 0 && parts[5].toInt() >= 0) parsedLength = parts[5].toInt();
    if (parts[6] == "1") parsedIsChannel = true;
    else if (parts[6] == "0") parsedIsChannel = false;

    pkt.timestamp_hex = parts[0];
    pkt.channel_name  = parts[1];
    pkt.channel_id    = parts[2];
    pkt.sender        = parts[3];
    pkt.message_id    = parts[4];
    pkt.length        = parsedLength;
    pkt.is_channel    = parsedIsChannel;
    pkt.message       = parts[7];
    pkt.valid         = true;
}

// ================== AODV FUNCTIONS ==================
void LoRaNode::sendDataAODV(const String &dest, const String &message) {
    if (routing_table.count(dest) && routing_table[dest].valid) {
        ParsedPacket pkt;
        pkt.sender = getAddress();
        pkt.message_id = String(millis());
        pkt.timestamp_hex = String(millis(), HEX);
        pkt.channel_name = "DATA";
        pkt.channel_id = dest;
        pkt.length = message.length();
        pkt.is_channel = false;
        pkt.message = message;
        sendMessage(pkt);
    } else {
        sendRREQ(dest);
    }
}

// ================== SEND RREQ ==================
void LoRaNode::sendRREQ(const String &dest) {
    broadcastCounter++;
    RREQPacket rreq;
    rreq.source = getAddress();
    rreq.destination = dest;
    rreq.source_seq = millis();
    rreq.dest_seq = 0;
    rreq.broadcast_id = broadcastCounter;
    rreq.hop_count = 0;
    rreq.ttl = 5; // configurable TTL

    ParsedPacket pkt;
    pkt.sender = getAddress();
    pkt.channel_name = "RREQ";
    pkt.channel_id = dest;
    pkt.message = String(rreq.source_seq) + "||" + String(rreq.dest_seq) + "||" +
                  String(rreq.broadcast_id) + "||" + String(rreq.hop_count) + "||" + String(rreq.ttl);
    pkt.length = pkt.message.length();
    pkt.is_channel = true;
    pkt.timestamp_hex = String(millis(), HEX);
    pkt.message_id = String(millis());
    sendMessage(pkt);
}

// ================== RECEIVE AODV ==================
void LoRaNode::receiveAODV(const ParsedPacket &pkt) {
    if (pkt.channel_name == "RREQ") {
        int src_seq, dst_seq, bcast_id, hop, ttl;
        sscanf(pkt.message.c_str(), "%d||%d||%d||%d||%d", &src_seq, &dst_seq, &bcast_id, &hop, &ttl);
        RREQPacket rreq{pkt.sender, pkt.channel_id, src_seq, dst_seq, bcast_id, hop, ttl};
        handleRREQ(rreq);
    } else if (pkt.channel_name == "RREP") {
        int dest_seq, hop;
        sscanf(pkt.message.c_str(), "%d||%d", &dest_seq, &hop);
        RREPPacket rrep{pkt.channel_id, pkt.sender, dest_seq, hop};
        handleRREP(rrep);
    }
}

// ================== HANDLE RREQ ==================
void LoRaNode::handleRREQ(const RREQPacket &rreq) {
    String key = rreq.source + "_" + String(rreq.broadcast_id);
    if (seen_broadcasts[key] >= rreq.broadcast_id) return;
    seen_broadcasts[key] = rreq.broadcast_id;

    if (!routing_table.count(rreq.source) || !routing_table[rreq.source].valid) {
        routing_table[rreq.source] = {rreq.source, rreq.source, rreq.hop_count + 1, rreq.source_seq, true, millis() + 60000};
    }

    if (getAddress() == rreq.destination) {
        sendRREP(rreq.source, rreq.source, 0, rreq.dest_seq);
        return;
    }

    if (rreq.ttl <= 0) return; // discard expired TTL

    // Rebroadcast with random delay
    RREQPacket newRREQ = rreq;
    newRREQ.hop_count++;
    newRREQ.ttl--;

    ParsedPacket pkt;
    pkt.sender = getAddress();
    pkt.channel_name = "RREQ";
    pkt.channel_id = rreq.destination;
    pkt.message = String(newRREQ.source_seq) + "||" + String(newRREQ.dest_seq) + "||" +
                  String(newRREQ.broadcast_id) + "||" + String(newRREQ.hop_count) + "||" + String(newRREQ.ttl);
    pkt.length = pkt.message.length();
    pkt.is_channel = true;
    pkt.timestamp_hex = String(millis(), HEX);
    pkt.message_id = String(millis());

    unsigned long delay_ms = random(10, 50);
    delay(delay_ms);
    sendMessage(pkt);
}

// ================== HANDLE RREP ==================
void LoRaNode::handleRREP(const RREPPacket &rrep) {
    routing_table[rrep.destination] = {rrep.destination, rrep.destination, rrep.hop_count,
                                       rrep.dest_seq, true, millis() + 60000};
}

// ================== SEND RREP ==================
void LoRaNode::sendRREP(const String &dest, const String &next_hop, int hop_count, int dest_seq) {
    ParsedPacket pkt;
    pkt.sender = getAddress();
    pkt.channel_name = "RREP";
    pkt.channel_id = dest;
    pkt.message = String(dest_seq) + "||" + String(hop_count);
    pkt.length = pkt.message.length();
    pkt.is_channel = true;
    pkt.timestamp_hex = String(millis(), HEX);
    pkt.message_id = String(millis());
    sendMessage(pkt);
}

// ================== LINK BREAK ==================
void LoRaNode::handleLinkBreak(const String &next_hop) {
    for (auto &entry : routing_table) {
        if (entry.second.next_hop == next_hop) entry.second.valid = false;
    }
}
