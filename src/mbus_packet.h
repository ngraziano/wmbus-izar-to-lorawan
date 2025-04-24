#ifndef MBUS_PACKET_H
#define MBUS_PACKET_H
#include <stdbool.h>
#include <stdint.h>

enum class PacketDecodeResult : uint8_t {
  OK = 0,
  CODING_ERROR = 1,
  CRC_ERROR = 2,
};

uint16_t packetSize(uint8_t lField);
PacketDecodeResult decodeRXBytesTmode(const uint8_t *pByte, uint8_t *pPacket, uint16_t packetSize);

#endif