#ifndef _3OUTOF6_H
#define _3OUTOF6_H
#include <stdint.h>

enum class DecodeResult : uint8_t {
    OK = 0,
    ERROR,
};

DecodeResult decode3outof6(const uint8_t *encodedData, uint8_t *decodedData, uint8_t lastByte);

#endif