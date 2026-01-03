#ifndef RADIO_H
#define RADIO_H

#include <Arduino.h>
#include <LoRa.h>
#include <map>

#define ROUTE_LIFETIME 60000
#define MAX_HOP 10

// ================== BASE PACKET (ON-AIR) ==================
struct Packet {
    String channel_id;
    String message_id;
    String sender_id;
    String message;   // contains DATA / RREQ / RREP payload
    bool valid;
};

// ================== LOGICAL AODV VIEW ==================
struct AODVPacket {
    String type;          // DATA, RREQ, RREP
    String destination;
    String sender;

    // RREQ
    unsigned long src_seq;
    unsigned long dst_seq;
    int broadcast_id;
    int hop_count;
    int ttl;

    // RREP
    unsigned long dest_seq;

    // DATA
    String data;

    bool valid;
};

// ================== ROUTING ==================
struct RouteEntry {
    String destination;
    String next_hop;
    int hop_count;
    unsigned long sequence_number;
    bool valid;
    unsigned long expiration_time;
};

class LoRaNode {
public:
    LoRaNode(String nodeAddress, int spreadingFactor,
             int sck = 5, int miso = 6, int mosi = 7,
             int ss  = 8, int rst  = 0, int dio0 = 1);

    bool begin(long frequency = 433E6);

    // messaging
    void sendMessage(const Packet &pkt);
    void processReceived(int packetSize);

    // AODV API
    void sendDataAODV(const String &dest, const String &message);
    void sendRREQ(const String &dest);
    void sendRREP(const String &dest, int hop_count, unsigned long dest_seq);

    void refreshAODVTable();
    void printRoutingTable();

    String getAddress() const { return address; }

private:
    String address;
    int sf;

    int pin_sck, pin_miso, pin_mosi, pin_ss, pin_rst, pin_dio0;

    Packet received_packet;

    std::map<String, RouteEntry> routing_table;
    std::map<String, int> seen_broadcasts;
    int broadcastCounter = 0;

    // internal helpers
    void parseRawPacket(const String &raw, Packet &pkt);
    bool parseAODVFromPacket(const Packet &pkt, AODVPacket &aodv);

    void handleAODV(const AODVPacket &pkt);
    void handleRREQ(const AODVPacket &pkt);
    void handleRREP(const AODVPacket &pkt);
};

#endif
