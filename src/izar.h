#ifndef IZAR_H
#define IZAR_H

#include <stdint.h>
#include <array>


// result format 
//  |   0    |   1    |   2    | 3 | 4 | 5 | 6 |
//  | flag 0 | flag 1 | flag 2 | index lsb     |

bool printAndExtractIZAR(const uint8_t *packet, const uint8_t length,
                    const std::array<uint8_t, 6> &wantedId,
                    std::array<uint8_t, 7> &result);

#endif