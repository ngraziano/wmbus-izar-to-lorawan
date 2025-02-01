#ifndef CRC_H
#define CRC_H
#include <stdint.h>

#define CRC_POLYNOM 0x3D65

class CrcCalc {
  uint16_t reg = 0;

public:
  void pushData(uint8_t data);
  void reset() { reg = 0; };
  bool checLow(uint8_t crcLow) const { return (~reg & 0xff) == crcLow; };
  bool checHigh(uint8_t crcHigh) const {
    return (((~reg) >> 8) & 0xff) == crcHigh;
  };
};

#endif