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
    bool valid;
};

class LoRaNode {
public:
    LoRaNode(String nodeAddress, int spreadingFactor,
             int sck = 5, int miso = 6, int mosi = 7,
             int ss  = 8, int rst  = 0, int dio0 = 1);

    bool begin(long frequency = 433E6);

    void sendMessage(const Packet &pkt);
    void processReceived(int packetSize);

    String getAddress() const { return address; }

private:
    String address;
    int sf;

    int pin_sck, pin_miso, pin_mosi, pin_ss, pin_rst, pin_dio0;

    Packet received_packet;

    void parseRawPacket(const String &raw, Packet &pkt);
};

#endif
