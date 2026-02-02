#ifndef GLOBALS_H
#define GLOBALS_H


#include <Arduino.h>
#include <vector>



// ================== PACKET ==================
/**
 * @brief Object to store packet data such as
 * channel_id, channel_name, sender_id, message_id, message, date_and_time,
 * rssi, snr, and latency
 * @param channel_id the id of the channel where this message is intended to be sent
 * @param channel_name the name of the target channel
 * @param sender_id the id of the sender node
 * @param message_id unique id of this packet
 * @param date_and_time yields date and time `String`
 * @param content the message contained within the packet
 * @param rssi calculated by a receving node
 * @param snr calculated by a receiving node
 * @param latency canculated by a receiving node through round trip time
 * @returns `struct Packet`
**/
struct Packet {
    unsigned long time_stamp;
    String date_and_time;
    String message_id;
    String sender_id;
    String channel_id;
    String sender_name;
    String channel_name;
    String content;
    int rssi;
    float snr;
    unsigned long latency;  // latency in milliseconds (roundtrip time)
    int receive_count;      // number of times this message has been received/retransmitted
    bool valid;
};

struct ackPacket {
    String message_id;
    String sender_id;
    int rssi;
    float snr;
    unsigned long latency;
    bool valid;
};



std::vector <String> messageIdSentLog;
std::vector <ackPacket> ackPacketLog;




#endif