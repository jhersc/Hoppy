// radio.h

/**
    @file radio.h
    @brief Handles reception and transmision of LoRa packets
**/

#ifndef RADIO_H
#define RADIO_H

#include <Arduino.h>
#include <LoRa.h>
#include <map>

#include "globals.h"

/**
 * @class LoRaNode
 * @return class LoRaNode
 * 
 */
class LoRaNode {
public:
    LoRaNode(String nodeAddress, int spreadingFactor,
             int sck = 13, int miso = 18, int mosi = 19,
             int ss  = 23, int rst  = 33, int dio0 = 32);

    bool begin(long frequency = 433E6);
    
    
    /**
     * @brief constructs a packet then sends it in the format
     * channel_id||channel_name||sender_id||
     * 
     */
    void sendToLoRa(const Packet &pkt);
    void sendUart(const Packet &pkt);
    void sendUartUpdate(const Packet &pkt);
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
    
    // Track sent message IDs with their sent_millis for latency calculation
    // message_id → sent_millis
    std::map<String, unsigned long> sentTimestamps;
    
    // Track message receive counts for retransmission logic
    // message_id → receive_count
    std::map<String, int> messageReceiveCount;
    static const unsigned long LATENCY_TIMEOUT = 30000;  // 30 seconds
    static const int MAX_RECEIVE_COUNT = 2;  // Allow retransmitting when not received up to 2 times

    void parseRawPacket(const String &raw, Packet &pkt);
    bool recentlySent(const String &msgId);
};

#endif
