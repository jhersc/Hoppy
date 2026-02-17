#include "preferencesHandler.h"


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

void storePacket(const PrefsPacket &outgoing) {
    String packet = 
        String(outgoing.date_and_time) + "||" +
        String(outgoing.message_id)    + "||" +
        String(outgoing.channel_id)    + "||" +
        String(outgoing.channel_name)  + "||" +
        String(outgoing.sender_name)   + "||" +
        String(outgoing.sender_id)     + "||" +
        String(outgoing.content)       + "||" +
        String(outgoing.rssi ? outgoing.rssi : 0) + "||" +
        String(outgoing.snr ? outgoing.snr : 0)   + "||" +
        String(outgoing.latency ? outgoing.latency : 0);
                    
    prefs.putString(("packet_" + String(outgoing.date_and_time)).c_str(), packet.c_str());
}

void PreferencesHandlerBegin() {
    prefs.begin("packets", false);

    if (!prefs.isKey("packet_names")) {
        prefs.putString("packet_names", "");
    }
}