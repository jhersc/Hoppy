#include "radio.h"

// ================== CONSTRUCTOR ==================
LoRaNode::LoRaNode(String nodeAddress, int spreadingFactor,
                   int sck, int miso, int mosi, int ss,
                   int rst, int dio0)
    : address(nodeAddress), sf(spreadingFactor), messageInterval(0), sendCounter(0),
      pin_sck(sck), pin_miso(miso), pin_mosi(mosi),
      pin_ss(ss), pin_rst(rst), pin_dio0(dio0) {}

// ================== INITIALIZATION ==================
bool LoRaNode::begin(long frequency) {
    SPI.begin(pin_sck, pin_miso, pin_mosi, pin_ss);
    LoRa.setPins(pin_ss, pin_rst, pin_dio0);

    if (!LoRa.begin(frequency)) {
        Serial.println("[FATAL] LoRa init failed.");
        return false;
    }

    LoRa.setSpreadingFactor(sf);
    Serial.println("[INFO] LoRa initialized successfully.");
    return true;
}

void LoRaNode::setMessageInterval(unsigned long ms) {
    messageInterval = ms;
}

// ================== SEND MESSAGE ==================
// Format (NEW):
// timestamp_hex || channel_name || channel_id || sender || message_id || length || is_channel || message
void LoRaNode::sendMessage(const ParsedPacket &pkt) {
    sendCounter++;

    // Build the new standardized packet format
    String packet =
        pkt.timestamp_hex + "||" +
        pkt.channel_name  + "||" +
        pkt.channel_id    + "||" +
        pkt.sender        + "||" +
        pkt.message_id    + "||" +
        String(pkt.length) + "||" +
        String(pkt.is_channel ? 1 : 0) + "||" +
        pkt.message;

    // Send via LoRa
    LoRa.beginPacket();
    LoRa.print(packet);
    LoRa.endPacket();

    // Log for debugging / monitoring
    Serial.println("[LoRa TX] " + packet);

    // Return to receive mode
    LoRa.receive();
}


// ================== RECEIVE MESSAGE ==================
void LoRaNode::processReceived(int packetSize) {
    if (packetSize <= 0) return;

    String raw;
    while (LoRa.available()) raw += (char)LoRa.read();

    if (raw.isEmpty()) {
        Serial.println("[D] Empty LoRa payload received.");
        return;
    }

    Serial.println("[D] RAW RX: " + raw);

    parseRawPacket(raw, received_packet);

    // Ignore packets from self
    if (received_packet.valid && received_packet.sender == address) {
        Serial.println("[D] Ignored self-packet.");
        received_packet.valid = false;
    }
}

// ================== PARSER ==================
// Expected format (8 fields):
// timestamp_hex || channel_name || channel_id || sender || message_id || length || is_channel || message
void LoRaNode::parseRawPacket(String raw, ParsedPacket &pkt) {
    // Start invalid; only set to true when everything is OK
    pkt.valid = false;

    raw.trim();
    if (raw.length() == 0) {
        Serial.println("[D] Empty raw packet");
        return;
    }

    // We expect 8 parts
    String parts[8];
    int lastIndex = 0;
    int partIndex = 0;

    // Parse 7 occurrences of "||" to extract first 7 fields, the rest is field 8 (message)
    for (; partIndex < 7; ++partIndex) {
        int sepIndex = raw.indexOf("||", lastIndex);
        if (sepIndex == -1) {
            Serial.println("[D] Malformed LoRa packet: expected 8 fields but found fewer.");
            return;
        }
        parts[partIndex] = raw.substring(lastIndex, sepIndex);
        parts[partIndex].trim();
        lastIndex = sepIndex + 2;
    }

    // Last field = remainder of string (message) — can contain "||"
    parts[7] = raw.substring(lastIndex);
    parts[7].trim();

    // Now assign to ParsedPacket fields (names follow your requested schema)
    // Defensive checks for numeric conversions
    String ts_hex     = parts[0];
    String ch_name    = parts[1];
    String ch_id      = parts[2];
    String sender     = parts[3];
    String msg_id     = parts[4];
    String length_str = parts[5];
    String is_ch_str  = parts[6];
    String message    = parts[7];

    // Validate numeric fields
    int parsedLength = 0;
    bool parsedIsChannel = false;

    // parse length
    if (length_str.length() > 0) {
        bool ok = true;
        for (size_t i = 0; i < length_str.length(); ++i) if (!isDigit(length_str[i])) { ok = false; break; }
        if (ok) parsedLength = length_str.toInt();
        else {
            Serial.println("[D] Malformed length field in LoRa packet.");
            return;
        }
    }

    // parse is_channel (expecting "0" or "1")
    if (is_ch_str == "1") parsedIsChannel = true;
    else if (is_ch_str == "0") parsedIsChannel = false;
    else {
        Serial.println("[D] Malformed is_channel field in LoRa packet (expect 0/1).");
        return;
    }

    // Assign values into the ParsedPacket structure (field names you used)
    // NOTE: adapt these member names if your ParsedPacket struct differs.
    pkt.timestamp_hex = ts_hex;
    pkt.channel_name  = ch_name;
    pkt.channel_id    = ch_id;
    pkt.sender        = sender;
    pkt.message_id    = msg_id;
    pkt.length        = parsedLength;       // keep the sender-declared length
    pkt.is_channel    = parsedIsChannel;
    pkt.message       = message;
    pkt.valid         = true;

    // Optional sanity: overwrite length with actual message length if you prefer
    // pkt.length = pkt.message.length();

    // Echo back (same 8-field format) so other MCU / hop can forward it unchanged
    if (pkt.valid) {
        String out = pkt.timestamp_hex + "||" +
                     pkt.channel_name  + "||" +
                     pkt.channel_id    + "||" +
                     pkt.sender        + "||" +
                     pkt.message_id    + "||" +
                     String(pkt.length)+ "||" +
                     String(pkt.is_channel ? 1 : 0) + "||" +
                     pkt.message;
        Serial.println(out);
    }
}
