#ifndef _3OUTOF6_H
#define _3OUTOF6_H
#include <stdint.h>

bool decode3outof6(const uint8_t *encodedData, uint8_t *decodedData, bool lastByte);

#endif