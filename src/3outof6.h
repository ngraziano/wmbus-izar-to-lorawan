#ifndef _3OUTOF6_H
#define _3OUTOF6_H
#include <stdint.h>

#define DECODING_3OUTOF6_OK      0
#define DECODING_3OUTOF6_ERROR   1

uint8_t decode3outof6(uint8_t *encodedData, uint8_t *decodedData, uint8_t lastByte);

#endif