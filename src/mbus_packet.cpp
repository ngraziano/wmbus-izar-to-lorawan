#include <stdint.h>

#include "3outof6.h"
#include "crc.h"
#include "mbus_packet.h"

/// @brief Returns the number of bytes in a Wireless MBUS packet from the
/// L-field. Note that the L-field excludes the L-field and the CRC fields
/// @param lField The L-field value in a Wireless MBUS packet
/// @return The number of bytes in a wireless MBUS packet
uint16_t packetSize(uint8_t lField) {
  // The 2 first blocks contains 25 bytes when excluding CRC and the L-field
  // The other blocks contains 16 bytes when excluding the CRC-fields
  // Less than 26 (15 + 10)
  uint8_t nrBlocks = 2;

  if (lField > 26)
    nrBlocks += 1 + ((lField - 26) / 16);

  // Add all extra fields, excluding the CRC fields
  uint16_t nrBytes = lField + 1;
  // Add the CRC fields, each block is contains 2 CRC bytes
  nrBytes += (2 * nrBlocks);

  return nrBytes;
}

/// @brief Decode a TMODE packet into a Wireless MBUS packet. Checks for 3 out
/// of 6 decoding errors and CRC errors.
/// @param pByte Pointer to TMBUS packet
/// @param pPacket Pointer to Wireless MBUS packet
/// @param packetSize Total Size of the Wireless MBUS packet (decoded size)
/// @return Error code
PacketDecodeResult decodeRXBytesTmode(const uint8_t *pByte, uint8_t *pPacket, uint16_t packetSize) {

  uint16_t bytesRemaining = packetSize;
  uint16_t bytesEncoded = 0;
  // Current CRC value
  CrcCalc crc = {};

  // Decode packet 2 byte at a time
  while (bytesRemaining > 1) {
    // Check for valid 3 out of 6 decoding
    if (!decode3outof6(pByte, pPacket, false))
      return PacketDecodeResult::CODING_ERROR;

    bytesRemaining -= 2;
    bytesEncoded += 2;

    // Current fields are a CRC field
    bool crcField = false;

    // Check if current field is CRC fields
    // - Field 10 + 18*n
    // - Less than 2 bytes
    if (bytesRemaining == 0)
      crcField = true;
    else if (bytesEncoded > 10)
      crcField = !((bytesEncoded - 12) % 18);

    // Check CRC field
    if (crcField) {
      if (!(crc.checkLow(*(pPacket + 1)) && crc.checkHigh(*pPacket)))
        return PacketDecodeResult::CRC_ERROR;
      crc = {};
    } else if (bytesRemaining == 1) {
      // If 1 bytes left, the field is the high byte of the CRC
      crc.pushData(*(pPacket));
      // The packet byte is a CRC-field
      if (!crc.checkHigh(*(pPacket + 1)))
        return PacketDecodeResult::CRC_ERROR;
    } else {
      // Perform CRC calculation
      crc.pushData(*(pPacket));
      crc.pushData(*(pPacket + 1));
    }

    pByte += 3;
    pPacket += 2;
  }

  // handle the last byte if necessary
  if (bytesRemaining == 1) {
    if (!decode3outof6(pByte, pPacket, true))
      return PacketDecodeResult::CODING_ERROR;
    // The last byte the low byte of the CRC field
    if (!crc.checkLow(*pPacket))
      return PacketDecodeResult::CRC_ERROR;
  }

  return PacketDecodeResult::OK;
}
