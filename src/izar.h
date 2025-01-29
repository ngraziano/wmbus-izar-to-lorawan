#ifndef IZAR_H
#define IZAR_H

#include <stdint.h>
#include <array>

bool printAndExtractIZAR(const uint8_t *packet, const uint8_t length,
                    const std::array<uint8_t, 6> &wantedId,
                    std::array<uint8_t, 7> &result);

#endif