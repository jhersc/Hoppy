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
        pkt.message;

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

    DBG("RX: " + raw);

    AODVPacket aodv;
    if (parseAODVFromPacket(received_packet, aodv)) {
        handleAODV(aodv);
    }
}

// ================== PARSERS ==================
void LoRaNode::parseRawPacket(const String &raw, Packet &pkt) {
    pkt.valid = false;

    int i1 = raw.indexOf("||");
    int i2 = raw.indexOf("||", i1 + 2);
    int i3 = raw.indexOf("||", i2 + 2);

    if (i1 < 0 || i2 < 0 || i3 < 0) return;

    pkt.channel_id = raw.substring(0, i1);
    pkt.message_id = raw.substring(i1 + 2, i2);
    pkt.sender_id  = raw.substring(i2 + 2, i3);
    pkt.message    = raw.substring(i3 + 2);
    pkt.valid = true;
}

bool LoRaNode::parseAODVFromPacket(const Packet &pkt, AODVPacket &aodv) {
    aodv.valid = false;

    int p = pkt.message.indexOf("||");
    if (p < 0) return false;

    aodv.type = pkt.message.substring(0, p);
    String body = pkt.message.substring(p + 2);
    aodv.sender = pkt.sender_id;

    if (aodv.type == "DATA") {
        int d = body.indexOf("||");
        if (d < 0) return false;
        aodv.destination = body.substring(0, d);
        aodv.data = body.substring(d + 2);
        aodv.valid = true;
    }

    else if (aodv.type == "RREQ") {
        sscanf(body.c_str(), "%[^|]||%lu||%lu||%d||%d||%d",
               aodv.destination.c_str(),
               &aodv.src_seq,
               &aodv.dst_seq,
               &aodv.broadcast_id,
               &aodv.hop_count,
               &aodv.ttl);
        aodv.valid = true;
    }

    else if (aodv.type == "RREP") {
        sscanf(body.c_str(), "%[^|]||%lu||%d",
               aodv.destination.c_str(),
               &aodv.dest_seq,
               &aodv.hop_count);
        aodv.valid = true;
    }

    return aodv.valid;
}

// ================== AODV SEND ==================
void LoRaNode::sendDataAODV(const String &dest, const String &message) {
    if (!routing_table.count(dest) || !routing_table[dest].valid) {
        INFO("No route to " + dest + ", sending RREQ");
        sendRREQ(dest);
        return;
    }

    Packet pkt;
    pkt.channel_id = "AODV";
    pkt.message_id = String(millis());
    pkt.sender_id  = address;
    pkt.message    = "DATA||" + dest + "||" + message;
    pkt.valid = true;

    sendMessage(pkt);
}

void LoRaNode::sendRREQ(const String &dest) {
    broadcastCounter++;

    Packet pkt;
    pkt.channel_id = "AODV";
    pkt.message_id = String(millis());
    pkt.sender_id  = address;
    pkt.message =
        "RREQ||" + dest + "||" +
        String(millis()) + "||0||" +
        String(broadcastCounter) + "||0||" +
        String(MAX_HOP);
    pkt.valid = true;

    sendMessage(pkt);
}

void LoRaNode::sendRREP(const String &dest, int hop, unsigned long seq) {
    Packet pkt;
    pkt.channel_id = "AODV";
    pkt.message_id = String(millis());
    pkt.sender_id  = address;
    pkt.message =
        "RREP||" + dest + "||" +
        String(seq) + "||" + String(hop);
    pkt.valid = true;

    sendMessage(pkt);
}

// ================== AODV HANDLERS ==================
void LoRaNode::handleAODV(const AODVPacket &pkt) {
    if (pkt.type == "RREQ") handleRREQ(pkt);
    else if (pkt.type == "RREP") handleRREP(pkt);
}

void LoRaNode::handleRREQ(const AODVPacket &pkt) {
    String key = pkt.sender + "_" + String(pkt.broadcast_id);
    if (seen_broadcasts[key] >= pkt.broadcast_id) return;
    seen_broadcasts[key] = pkt.broadcast_id;

    routing_table[pkt.sender] = {
        pkt.sender,
        pkt.sender,
        pkt.hop_count + 1,
        pkt.src_seq,
        true,
        millis() + ROUTE_LIFETIME
    };

    if (pkt.destination == address) {
        INFO("Destination reached, sending RREP");
        sendRREP(pkt.sender, 0, pkt.dst_seq);
        return;
    }

    if (pkt.ttl <= 0) return;
    sendRREQ(pkt.destination);
}

void LoRaNode::handleRREP(const AODVPacket &pkt) {
    routing_table[pkt.destination] = {
        pkt.destination,
        pkt.sender,
        pkt.hop_count,
        pkt.dest_seq,
        true,
        millis() + ROUTE_LIFETIME
    };
}

// ================== ROUTE MAINT ==================
void LoRaNode::refreshAODVTable() {
    unsigned long now = millis();
    for (auto it = routing_table.begin(); it != routing_table.end();) {
        if (it->second.expiration_time < now)
            it = routing_table.erase(it);
        else ++it;
    }
}

void LoRaNode::printRoutingTable() {
    Serial.println("\n--- ROUTING TABLE (" + address + ") ---");
    for (auto &e : routing_table) {
        Serial.println(
            e.first + " via " + e.second.next_hop +
            " hops=" + e.second.hop_count
        );
    }
}
