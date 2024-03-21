// ExpandedCastMessageSerializer.hpp

#ifndef EXPANDED_CAST_MESSAGE_SERIALIZER_HPP
#define EXPANDED_CAST_MESSAGE_SERIALIZER_HPP

#include <stdint.h>
#include <string.h>

namespace ExpandedCastMessageSerializer {

const uint8_t MAX_STRING_SIZE = 64;
const uint8_t MAX_BINARY_PAYLOAD_SIZE = 64;

enum DeserializationResult {
  DESERIALIZATION_SUCCESS = 0,
  INVALID_FORMAT = 1,
  BUFFER_UNDERFLOW = 2
};

enum ProtocolVersion {
  CASTV2_1_0 = 0
};

enum PayloadType {
  STRING = 0,
  BINARY = 1
};

struct CastMessage {
  ProtocolVersion protocol_version;
  char source_id[MAX_STRING_SIZE];
  char destination_id[MAX_STRING_SIZE];
  char namespace_[MAX_STRING_SIZE];
  PayloadType payload_type;
  char payload_utf8[306];
  uint8_t payload_binary[MAX_BINARY_PAYLOAD_SIZE];
  uint8_t payload_binary_size;  // Actual size of binary payload
};

enum SerializationResult {
  SUCCESS = 0,
  BUFFER_OVERFLOW = 1
};

inline DeserializationResult deserialize(const uint8_t* buffer, uint16_t length, char* payloadUtf8) {
  uint16_t index = 4;  // Start after the 4-byte length field

  Serial.println("Initial length: " + String(length));

  // First, let's attempt to find the session ID
  const char* sessionIdKey = "\"sessionId\":\"";
  const char* sessionIdStart = strstr((const char*)buffer + index, sessionIdKey);

  if (sessionIdStart) {
    sessionIdStart += strlen(sessionIdKey);  // Move to the start of the sessionId value
    const char* sessionIdEnd = strchr(sessionIdStart, '\"');

    if (sessionIdEnd) {
      int sessionIdLength = sessionIdEnd - sessionIdStart;
      memcpy(payloadUtf8, sessionIdStart, sessionIdLength);
      payloadUtf8[sessionIdLength] = '\0';  // Null-terminate

      Serial.println("Extracted sessionId: " + String(payloadUtf8));
      return ExpandedCastMessageSerializer::DESERIALIZATION_SUCCESS;  // Indicate that only part of the message was deserialized
    }
  }

  // If we are here, it means we didn't find a session ID.
  // Let's now search for a valid JSON payload.

  int payloadStart = -1;
  int braceCount = 0;

  for (; index < length; index++) {
    if (buffer[index] == '{' && payloadStart == -1) {
      payloadStart = index;
      braceCount = 1;
    } else if (buffer[index] == '{') {
      braceCount++;
    } else if (buffer[index] == '}') {
      braceCount--;
    }

    if (payloadStart != -1 && braceCount == 0) {
      break;
    }
  }

  if (payloadStart != -1 && braceCount == 0) {
    int payloadLength = index - payloadStart + 1;

    Serial.println("Identified JSON payload from index " + String(payloadStart) + " to " + String(index));
    Serial.println("Payload length (after identification): " + String(payloadLength));

    // Copy the identified payload
    memcpy(payloadUtf8, &buffer[payloadStart], payloadLength);
    payloadUtf8[payloadLength] = '\0';  // Null-terminate

    return ExpandedCastMessageSerializer::DESERIALIZATION_SUCCESS;
  } else {
    Serial.println(F("Failed to identify complete JSON payload or sessionId within buffer"));
    return ExpandedCastMessageSerializer::INVALID_FORMAT;
  }
}

SerializationResult serialize(const CastMessage& message, uint8_t* buffer, uint16_t& index, uint16_t bufferSize) {
  auto addString = [&](const char* str, uint8_t fieldNumber) {
    uint16_t length = strlen(str);
    if (index + length + 3 > bufferSize) return BUFFER_OVERFLOW;  // +3 for potential longer varint
    buffer[index++] = (fieldNumber << 3) | 2;  // Length-delimited
    // Encode the length as varint
    while (length >= 0x80) {
      buffer[index++] = (length & 0x7F) | 0x80;
      length >>= 7;
    }
    buffer[index++] = length;
    memcpy(&buffer[index], str, length);
    index += length;
    return SUCCESS;
  };

  // Protocol Version
  if (index + 2 > bufferSize) return BUFFER_OVERFLOW;
  buffer[index++] = 0x08;  // Field 1, Varint
  buffer[index++] = 0x00;  // CASTV2_1_0
  
  // Source ID
  if (addString(message.source_id, 2) != SUCCESS) return BUFFER_OVERFLOW;
  
  // Destination ID
  if (addString(message.destination_id, 3) != SUCCESS) return BUFFER_OVERFLOW;

  // Namespace
  if (addString(message.namespace_, 4) != SUCCESS) return BUFFER_OVERFLOW;

  // Payload Type
  if (index + 2 > bufferSize) return BUFFER_OVERFLOW;
  buffer[index++] = 0x28;  // Field 5, Varint
  buffer[index++] = 0x00;  // STRING

  // Payload
  if (addString(message.payload_utf8, 6) != SUCCESS) return BUFFER_OVERFLOW;

  Serial.println("Serialized length: " + String(index));

  return SUCCESS;
}

}  // namespace ExpandedCastMessageSerializer

#endif  // EXPANDED_CAST_MESSAGE_SERIALIZER_HPP