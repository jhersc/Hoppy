#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

// ================== COMMON PACKET STRUCT ==================
// Represents a single message packet sent or received
// ================== PARSED PACKET ==================
struct ParsedPacket {
    String timestamp_hex;
    String channel_name;
    String channel_id;
    String sender;
    String message_id;
    int length;
    bool is_channel;
    String message;
    bool valid;
};


// ================== LoRa Node Class ==================
// Encapsulates a LoRa node, handling send/receive logic
class LoRaNode {
public:
    // Constructor: initialize node with address, spreading factor, and pins
    LoRaNode(String nodeAddress, int spreadingFactor,
             int sck = 13, int miso = 18, int mosi = 19, int ss = 23,
             int rst = 33, int dio0 = 32);

    // Initialize LoRa module with optional frequency (default 433 MHz)
    bool begin(long frequency = 433E6);

    // Set minimum interval between outgoing messages (ms)
    void setMessageInterval(unsigned long ms);

    // Send a ParsedPacket via LoRa
    void sendMessage(const ParsedPacket &pkt);

    // Process a received packet of given size
    void processReceived(int packetSize);

    // Access the last received packet
    ParsedPacket getLastReceivedPacket() const { return received_packet; }

    // Clear the last received packet
    void clearLastReceivedPacket() { received_packet = ParsedPacket(); }

    // Get this node's unique address
    String getAddress() const { return address; }

private:
    // Parse a raw LoRa string into ParsedPacket
    void parseRawPacket(String raw, ParsedPacket &pkt);

    // Node properties
    String address;            // Node ID/address
    int sf;                     // Spreading factor (LoRa)
    unsigned long messageInterval; // Minimum interval between messages
    int sendCounter;            // Tracks outgoing message count

    // LoRa hardware pins
    int pin_sck, pin_miso, pin_mosi, pin_ss, pin_rst, pin_dio0;

    // Stores the last received message
    ParsedPacket received_packet;
};
