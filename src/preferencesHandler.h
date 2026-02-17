#pragma once
#ifndef PREFERENCES_HANDLER
#define PREFERENCES_HANDLER
#include <Arduino.h>
#include <Preferences.h>


struct PrefsPacket {
    // PACKET DATA
    char date_and_time[20];
    char sender_name[16];
    char channel_name[16];
    char channel_id[8];
    char sender_id[8];
    char message_id[8];
    char content[64];
    int rssi;
    float snr;
    float latency;
    bool valid;
};


extern Preferences prefs;

String generateUniqueId();
void storePacket(const PrefsPacket &outgoing);
void PreferencesHandlerBegin();

#endif