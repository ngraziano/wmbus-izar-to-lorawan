#ifndef CRC_H
#define CRC_H
#include <stdint.h>

class CrcCalc final {
  static constexpr uint16_t CRC_POLYNOM = 0x3D65;

  uint16_t reg = 0;

public:
  void pushData(uint8_t data);
  bool checkLow(uint8_t crcLow) const { return (~reg & 0xff) == crcLow; };
  bool checkHigh(uint8_t crcHigh) const { return (((~reg) >> 8) & 0xff) == crcHigh; };
};

#endif