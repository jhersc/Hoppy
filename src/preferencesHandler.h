#include <Arduino.h>
#include <Preferences.h>


struct Packet {
    // PACKET DATA
    char time_sent[20];
    char channel_name[16];
    char channel_id[8];
    char sender_id[8];
    char message_id[8];
    int length;
    bool is_channel;
    char message[64];

    // METADATA
    uint8_t spreading_factor;
    short rssi;
    float snr;
    char time_received[20];
    float latency_ms;
    bool valid;
};


Preferences prefs;

String generateUniqueId() {
    // 8 hex digits from millis + 4 hex digits random + 4 hex digits micros
    char buf[17];  // 16 chars + null terminator
    unsigned long t = millis();
    unsigned int r = random(0xFFFF);
    unsigned int u = micros() & 0xFFFF; // lower 16 bits for extra entropy
    sprintf(buf, "%08lX%04X%04X", t, r, u);
    return String(buf); // always 16 hex chars
}


void storePacket(const Packet &pkt) {
    String packet = String(pkt.time_sent) + "||" +
                    String(pkt.channel_name) + "||" +
                    String(pkt.channel_id) + "||" +
                    String(pkt.sender_id) + "||" +
                    String(pkt.message_id) + "||" +
                    String(pkt.length) + "||" +
                    String(pkt.is_channel ? 1 : 0) + "||" +
                    String(pkt.message) + "||" +
                    String(pkt.spreading_factor) + "||" +
                    String(pkt.rssi) + "||" +
                    String(pkt.snr) + "||" +
                    String(pkt.time_received) + "||" +
                    String(pkt.latency_ms) + "||" +
                    String(pkt.valid ? 1 : 0);
                    
    prefs.putString(("packet_" + generateUniqueId()).c_str(), packet.c_str());
}

void PreferencesHandlerBegin() {
    prefs.begin("packets", false);

    if (!prefs.isKey("packet_names")) {
        prefs.putString("packet_names", "");
    }
}

