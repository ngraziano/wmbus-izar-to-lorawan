#include "crc.h"

// Calculates the 16-bit CRC. The function requires that the CRC_POLYNOM is
// defined, which gives the wanted CRC polynom.
void CrcCalc::pushData(uint8_t data) {
  for (uint8_t i = 0; i < 8; i++) {
    // If upper most bit is 1
    if (((reg & 0x8000) >> 8) ^ (data & 0x80))
      reg = (reg << 1) ^ CRC_POLYNOM;
    else
      reg = (reg << 1);

    data <<= 1;
  }
}

