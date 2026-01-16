#ifndef RADIO_H
#define RADIO_H

#include <Arduino.h>
#include <LoRa.h>
#include <map>

#define SEEN_TIMEOUT 60000   // 1 minute

struct Packet {
    String channel_id;
    String message_id;
    String sender_id;
    String message;
    String time_stamp;
    bool valid;
};

class LoRaNode {
public:
    // LoRaNode(String nodeAddress, int spreadingFactor,
    //          int sck = 13, int miso = 18, int mosi = 19,
    //          int ss  = 23, int rst  = 33    , int dio0 = 32);
    LoRaNode(String nodeAddress, int spreadingFactor,
             int sck = 14, int miso = 12, int mosi = 13,
             int ss  = 15, int rst  = 16, int dio0 = 4);


    bool begin(long frequency = 433E6);

    void sendMessage(const Packet &pkt);
    void processReceived(int packetSize);

    void cleanupSeen();
    String getAddress() const { return address; }

private:
    String address;
    int sf;

    int pin_sck, pin_miso, pin_mosi, pin_ss, pin_rst, pin_dio0;

    Packet received_packet;

    // message_id → timestamp
    std::map<String, unsigned long> seenMessages;

    void parsePacket(const String &raw, Packet &pkt);
    bool alreadySeen(const String &msgId);
};

#endif
