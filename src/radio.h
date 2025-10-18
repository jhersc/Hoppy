#ifndef RADIO_H
#define RADIO_H

#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <map>
#include <vector>

// ================== PARSED PACKET ==================
struct ParsedPacket {
    String timestamp_hex;
    String channel_name;
    String channel_id;
    String sender;
    String message_id;
    int length;
    bool is_channel;
    String message;
    bool valid;
};

// ================== ROUTING STRUCTS ==================
struct RouteEntry {
    String destination;
    String next_hop;
    int hop_count;
    int sequence_number;
    bool valid;
    unsigned long expiration_time;
};

struct RREQPacket {
    String source;
    String destination;
    int source_seq;
    int dest_seq;
    int broadcast_id;
    int hop_count;
    int ttl;  // Time-to-live for RREQ
};

struct RREPPacket {
    String source;
    String destination;
    int dest_seq;
    int hop_count;
};

// ================== LoRa Node ==================
class LoRaNode {
public:
    LoRaNode(String nodeAddress, int spreadingFactor,
             int sck = 13, int miso = 18, int mosi = 19,
             int ss = 23, int rst = 33, int dio0 = 32);

    bool begin(long frequency = 433E6);
    void setMessageInterval(unsigned long ms);
    void sendMessage(const ParsedPacket &pkt);
    void processReceived(int packetSize);

    ParsedPacket getLastReceivedPacket() const { return received_packet; }
    void clearLastReceivedPacket() { received_packet = ParsedPacket(); }
    String getAddress() const { return address; }

    // ================== AODV FUNCTIONS ==================
    void sendDataAODV(const String &dest, const String &message);
    void receiveAODV(const ParsedPacket &pkt);
    void handleRREQ(const RREQPacket &rreq);
    void handleRREP(const RREPPacket &rrep);
    void sendRREQ(const String &dest);
    void sendRREP(const String &dest, const String &next_hop, int hop_count, int dest_seq);
    void handleLinkBreak(const String &next_hop);

private:
    void parseRawPacket(String raw, ParsedPacket &pkt);

    String address;
    int sf;
    unsigned long messageInterval;
    int sendCounter;

    int pin_sck, pin_miso, pin_mosi, pin_ss, pin_rst, pin_dio0;
    ParsedPacket received_packet;

    // ================== AODV MEMBERS ==================
    std::map<String, RouteEntry> routing_table;
    std::map<String, int> seen_broadcasts;
    int broadcastCounter = 0;
};

#endif
