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
    float latency;  // latency in seconds (roundtrip time)
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

void parseRawPacket(const String &raw, Packet &pkt) {
    pkt.valid = false;
    pkt.latency = 0;
    pkt.receive_count = 0;
        /**
     * msg||date_and_time||message_id||sender_id||
     * channel_id||sender_name||channel_name||content||rssi||snr||latency
     *  */ 
    String parts[9];
    int index = 0;
    String r = raw;
    while (r.length() > 0 && index < 8) {
        int sepIndex = r.indexOf("||");
        if (sepIndex == -1) {
            parts[index++] = r;
            break;
        } else {
            parts[index++] = r.substring(0, sepIndex);
            r = r.substring(sepIndex + 2);
        }
    }
    if (index < 9) return;

    pkt.date_and_time = parts[0];
    pkt.message_id    = parts[1];
    pkt.sender_id     = parts[2];
    pkt.channel_id    = parts[3];
    pkt.sender_name   = parts[4];
    pkt.channel_name  = parts[5];
    pkt.content       = parts[6];
    pkt.rssi          = parts[7].toInt();
    pkt.snr           = parts[8].toFloat();
    pkt.latency       = parts[9].toFloat();
    pkt.valid         = true;
}

void packetToPrefs(Packet &pkt, PrefsPacket &ppkt) {
    pkt.valid = false;
    pkt.latency = 0;
        /**
     * msg||date_and_time||message_id||sender_id||
     * channel_id||sender_name||channel_name||content||rssi||snr||latency
     *  */ 
    pkt.date_and_time.toCharArray(ppkt.date_and_time, sizeof(ppkt.date_and_time));
    pkt.sender_name.toCharArray(ppkt.sender_name, sizeof(ppkt.sender_name));
    pkt.channel_name.toCharArray(ppkt.channel_name, sizeof(ppkt.channel_name));
    pkt.channel_id.toCharArray(ppkt.channel_id, sizeof(ppkt.channel_id));
    pkt.sender_id.toCharArray(ppkt.sender_id, sizeof(ppkt.sender_id));  
    pkt.message_id.toCharArray(ppkt.message_id, sizeof(ppkt.message_id));
    pkt.content.toCharArray(ppkt.content, sizeof(ppkt.content));
    ppkt.rssi          = pkt.rssi;
    ppkt.snr           = pkt.snr;
    ppkt.latency       = pkt.latency;
    ppkt.valid         = true;
}




void markAsSeen(const String &msgId) {
    seenMessages[msgId] = millis();
}

void markAsSent(const String &msgId) {
    sentMessages[msgId] = millis();
}

bool alreadySeen(const String &msgId) {
    return seenMessages.count(msgId) > 0;
}

bool recentlySent(const String &msgId) {
    if (sentMessages.count(msgId)) return true;
    return false;
}


/**
 * @brief (key, value)
 * @param key message ID
 * @param value message time stamp
 */
std::map <String, String> sentMessages;
/**
 * @brief (key, value)
 * @param key message ID
 * @param value message time stamp
 */
std::map <String, String> seenMessages;






#endif