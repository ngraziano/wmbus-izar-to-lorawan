#include "izar.h"
#include <algorithm>
#include <array>
#include <lmic/bufferpack.h>
#include <stdint.h>
#include <stdio.h>

bool decodeDiehlLfsr(const uint8_t *const origin, uint8_t *const decoded,
                     const uint16_t size, uint32_t key);

void printHex(const uint8_t *packet, const uint8_t length) {
  for (uint8_t i = 0; i < length; i++) {
    printf("%02X", packet[i]);
  }
}

// assume only one frame
// Print and extract data with a lot of hardcoded values
bool printAndExtractIZAR(const uint8_t *packet, const uint8_t length,
                         const std::array<uint8_t, 6> &wantedId,
                         std::array<uint8_t, 7> &result) {
  // L field
  if (packet[0] != 0x19 && length != 30) {
    return false;
  }
  // C Field : periodic data brodcat
  if (packet[1] != 0x44) {
    return false;
  }

  // M field SAP
  if (packet[2] != 0x30 && packet[3] != 0x4C) {
    return false;
  }

  // A field :  ID +dim
  if (LMIC_DEBUG_LEVEL > 0) {
    printf("ID: ");
    printHex(&packet[4], 6);
  }

  // CRC byte 10 et 11
  // CI byte 12 = 0xA1 PRIOS
  if (packet[2 + 10] != 0xA1) {
    return false;
  }

  if (LMIC_DEBUG_LEVEL > 0) {
    printf(" Status: ");
    printHex(&packet[2 + 11], 3);
  }
  std::copy_n(packet + 2 + 11, 3, result.begin());
  //  2+14 : unit in liters
  if (packet[2 + 14] != 0x13) {
    return false;
  }

  // coded part [2+15] to [2+26] 11 bytes
  uint32_t key = 0x39BC8A10 ^ 0xE66D83F8;
  uint8_t decoded[11];
  if (!decodeDiehlLfsr(packet, decoded, 11, key)) {
    return false;
  }

  if (LMIC_DEBUG_LEVEL > 0) {
    uint32_t idx = rlsbf4(decoded + 1);
    printf(" Idx: %ld.%ld", idx / 1000L, idx % 1000L);
  }
  std::copy_n(decoded + 1, 4, result.begin() + 3);

  // return true only if it is the wanted counter
  return std::equal(wantedId.cbegin(), wantedId.cend(), packet + 4);
}

bool decodeDiehlLfsr(const uint8_t *const origin, uint8_t *const decoded,
                     const uint16_t size, uint32_t key) {
  // modify seed key with header values
  // manufacturer + address[0-1]
  key ^= rmsbf4(origin + 2);
  // address[2-3] + version + type
  key ^= rmsbf4(origin + 6);
  // ci + some more bytes from the telegram...
  key ^= rmsbf4(origin + 2 + 10);

  for (uint16_t i = 0; i < size; ++i) {
    // calculate new key (LFSR)
    // https://en.wikipedia.org/wiki/Linear-feedback_shift_register
    for (int j = 0; j < 8; ++j) {
      // calculate new bit value (xor of selected bits from previous key)
      uint8_t bit = ((key & 0x2) != 0) ^ ((key & 0x4) != 0) ^
                    ((key & 0x800) != 0) ^ ((key & 0x80000000) != 0);
      // shift key bits and add new one at the end
      key = (key << 1) | bit;
    }
    // decode i-th content byte with fresh/last 8-bits of key
    decoded[i] = origin[i + 15 + 2] ^ (key & 0xFF);
  }
  // check-byte does match
  return decoded[0] == 0x4B;
}
