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

//    Returns the total number of encoded bytes to receive or transmit, given
//    the total number of bytes in a Wireless MBUS packet. In receive mode the
//    postamble sequence and synchronization word is excluded from the
//    calculation.
//
//  ARGUMENTS:
//    uint16_t  packetSize  - Total number of bytes in the wireless MBUS packet
//
//  RETURNS
//    uint16_t  - The number of bytes of the encoded WMBUS packet
uint16_t byteSize(uint16_t packetSize) {

  uint16_t tmodeVar = (3 * packetSize) / 2;

  // Receive mode
  // If packetsize is a odd number 1 extra byte
  // that includes the 4-postamble sequence must be
  // read.
  if (packetSize % 2)
    return (tmodeVar + 1);
  else
    return (tmodeVar);
}

/// @brief Decode a TMODE packet into a Wireless MBUS packet. Checks for 3 out
/// of 6 decoding errors and CRC errors.
/// @param pByte Pointer to TMBUS packet
/// @param pPacket Pointer to Wireless MBUS packet
/// @param packetSize Total Size of the Wireless MBUS packet
/// @return Error code
PacketDecodeResult decodeRXBytesTmode(const uint8_t *pByte, uint8_t *pPacket,
                                      uint16_t packetSize) {

  uint16_t bytesRemaining = packetSize;
  uint16_t bytesEncoded = 0;
  CrcCalc crc = CrcCalc(); // Current CRC value

  // Decode packet
  while (bytesRemaining > 0) {
    // Check for valid 3 out of 6 decoding
    if (!decode3outof6(pByte, pPacket, bytesRemaining == 1))
      return PacketDecodeResult::CODING_ERROR;

    // If last byte
    if (bytesRemaining == 1) {
      bytesRemaining -= 1;
      bytesEncoded += 1;
      // The last byte the low byte of the CRC field
      if (!crc.checLow(*pPacket))
        return PacketDecodeResult::CRC_ERROR;
    }

    else {

      bytesRemaining -= 2;
      bytesEncoded += 2;

      bool crcField = false; // Current fields are a CRC field

      // Check if current field is CRC fields
      // - Field 10 + 18*n
      // - Less than 2 bytes
      if (bytesRemaining == 0)
        crcField = true;
      else if (bytesEncoded > 10)
        crcField = !((bytesEncoded - 12) % 18);

      // Check CRC field
      if (crcField) {
        if (!crc.checLow(*(pPacket + 1)))
          return PacketDecodeResult::CRC_ERROR;
        if (!crc.checHigh(*pPacket))
          return PacketDecodeResult::CRC_ERROR;

        crcField = false;
        crc.reset();
      }

      // If 1 bytes left, the field is the high byte of the CRC
      else if (bytesRemaining == 1) {
        crc.pushData(*(pPacket));
        // The packet byte is a CRC-field
        if (!crc.checHigh(*(pPacket + 1)))
          return PacketDecodeResult::CRC_ERROR;
      }

      // Perform CRC calculation
      else {
        crc.pushData(*(pPacket));
        crc.pushData(*(pPacket + 1));
      }

      pByte += 3;
      pPacket += 2;
    }
  }

  return PacketDecodeResult::OK;
}
