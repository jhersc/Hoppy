#ifndef RADIO_H
#define RADIO_H

#include <Arduino.h>
#include <LoRa.h>
#include <map>

#define SEEN_TIMEOUT 60000   // 1 minute

// ================== PACKET ==================
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
    LoRaNode(String nodeAddress, int spreadingFactor,
             int sck = 14, int miso = 12, int mosi = 12,
             int ss  = 15, int rst  = 16, int dio0 = 2);

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

    // message_id → timestamp (for flood control)
    std::map<String, unsigned long> seenMessages;

    void parseRawPacket(const String &raw, Packet &pkt);
    bool alreadySeen(const String &msgId);
};

#endif
