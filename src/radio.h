#ifndef RADIO_H
#define RADIO_H

#include <Arduino.h>
#include <LoRa.h>
#include <map>

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
             int sck = 13, int miso = 18, int mosi = 19,
             int ss  = 23, int rst  = 33, int dio0 = 32);

    bool begin(long frequency = 433E6);

    void sendMessage(const Packet &pkt);
    void processReceived(int packetSize);
    void cleanupSeenMessages();
    void markMessageSent(const String &msgId);
    void markAsSeen(const String &msgId);
    bool alreadySeen(const String &msgId);

    String getAddress() const { return address; }

private:
    String address;
    int sf;

    int pin_sck, pin_miso, pin_mosi, pin_ss, pin_rst, pin_dio0;

    Packet received_packet;
    
    // message_id → timestamp (for deduplication)
    std::map<String, unsigned long> seenMessages;
    static const unsigned long SEEN_TIMEOUT = 60000;  // 1 minute
    
    // Track recently sent message IDs to avoid immediate self-echo
    std::map<String, unsigned long> sentMessages;
    static const unsigned long SENT_TIMEOUT = 2000;  // 2 seconds

    void parseRawPacket(const String &raw, Packet &pkt);
    bool recentlySent(const String &msgId);
};

#endif
