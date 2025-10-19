#ifndef RADIO_H
#define RADIO_H

#include <Arduino.h>
#include <LoRa.h>
#include <map>

#define ROUTE_LIFETIME 60000
#define MAX_HOP 10

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

struct RouteEntry {
    String destination;
    String next_hop;
    int hop_count;
    unsigned long sequence_number;
    bool valid;
    unsigned long expiration_time;
};

struct RREQPacket {
    String source;
    String destination;
    unsigned long source_seq;
    unsigned long dest_seq;
    int broadcast_id;
    int hop_count;
    int ttl;
};

struct RREPPacket {
    String destination;
    String source;
    unsigned long dest_seq;
    int hop_count;
};

class LoRaNode {
public:
    LoRaNode(String nodeAddress, int spreadingFactor,
             int sck = 13, int miso = 18, int mosi = 19,
             int ss  = 23, int rst  = 33, int dio0 = 32);

    bool begin(long frequency = 915E6);
    void setMessageInterval(unsigned long ms);

    void sendMessage(const ParsedPacket &pkt);
    void processReceived(int packetSize);

    void sendDataAODV(const String &dest, const String &message);
    void receiveAODV(const ParsedPacket &pkt);
    void handleRREQ(const RREQPacket &rreq);
    void handleRREP(const RREPPacket &rrep);
    void sendRREQ(const String &dest);
    void sendRREP(const String &dest, const String &next_hop, int hop_count, int dest_seq); // changed to int
    void handleLinkBreak(const String &next_hop);
    void refreshAODVTable();

    void printRoutingTable();

    // === NEW helper accessors ===
    ParsedPacket getLastReceivedPacket() const { return received_packet; }
    void clearLastReceivedPacket() { received_packet.valid = false; }

    String getAddress() const { return address; }

private:
    String address;
    int sf;
    unsigned long messageInterval;
    unsigned long sendCounter;

    int pin_sck, pin_miso, pin_mosi, pin_ss, pin_rst, pin_dio0;

    ParsedPacket received_packet;

    std::map<String, RouteEntry> routing_table;
    std::map<String, int> seen_broadcasts;
    int broadcastCounter = 0;

    void parseRawPacket(String raw, ParsedPacket &pkt);
};

#endif
