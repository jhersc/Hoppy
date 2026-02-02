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
        outgoing.content       + "||" +
        String(outgoing.rssi ? outgoing.rssi : -1) + "||" +
        String(outgoing.snr ? outgoing.snr : -1)   + "||" +
        String(outgoing.latency ? outgoing.latency : -1);
        
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
void LoRaNode::sendToController(const Packet &pkt) {
    Packet outgoing = pkt;Packet outgoing = pkt;
    outgoing.sender_id = address;
    
    
    String raw =
    outgoing.date_and_time + "||" +
    outgoing.message_id    + "||" +
    outgoing.channel_id    + "||" +
    outgoing.channel_name  + "||" +
    outgoing.sender_name   + "||" +
    outgoing.sender_id     + "||" +
    outgoing.content       + "||" +
    String(outgoing.rssi ? outgoing.rssi : -1) + "||" +
    String(outgoing.snr ? outgoing.snr : -1)   + "||" +
    String(outgoing.latency ? outgoing.latency : -1);

    DBG("UART_TX: " + raw);
    Serial.println(raw); // sends to other MCU
    markAsSent(pkt.message_id);
    INFO(pkt.message_id + " MARKED AS SENT");
    return;        

}

/**
 * Process a received LoRa Packet
 * Sends it to LoRa and UART as needed
 */
void LoRaNode::processReceived(int packetSize) {
    // ignore empty packets
    if (packetSize <= 0) return;

    String raw;
    while (LoRa.available()) raw += (char)LoRa.read();
    DBG("RX: " + raw);
    // capture snr and rssi
    received_packet.snr  = LoRa.packetSnr();
    received_packet.rssi = LoRa.packetRssi();  
    parseRawPacket(raw, received_packet);
    //skip invalid packets
    if (!received_packet.valid) return;
    // skip seen messages
    if (seenMessages.count(received_packet.message_id)) return;
    
    // Calculate latency only for our OWN messages (when we receive our own transmission back)
    if (received_packet.sender_id == address) {
        if (sentMessages.count(received_packet.message_id)) {
            unsigned long sent_time = sentMessages[received_packet.message_id];
            received_packet.latency = millis() - sent_time;
            DBG("RX: Latency for our message " + received_packet.message_id + " = " + String(received_packet.latency) + "ms");
        }
        return;
        // DBG("RX (self): " + raw);
    }
    
    // rebroadcast
    sendToController(received_packet);
    sendToLoRa(received_packet);
    if (received_packet.latency > 0) {
        markAsSeen(received_packet.message_id);
        PrefsPacket pref;
        packetToPrefs(received_packet, pref);
        storePacket(pref);
        return;
    }
    return;
}
