#ifndef radio1276FSK_h
#define radio1276FSK_h

#include "hal/hal_io.h"
#include <array>
#include <stdint.h>

constexpr uint8_t IZAR_LENGH_3OUTOF6 = 0x45;
constexpr uint8_t IZAR_LENGH = 30;

class RadioSx1276FSK final {
public:
  explicit RadioSx1276FSK(lmic_pinmap const &pins,
                          const std::array<uint8_t, 6> &meter_id);
  bool listen_wmbus(std::array<uint8_t, 7> &result);
  void stop_listen();

private:
  void init();
  void handle_payload_ready();
  void handle_fifo_level();

  const std::array<uint8_t, 6> &meter_id;
  HalIo hal;
  bool listening = false;
  std::array<uint8_t, IZAR_LENGH_3OUTOF6> buffer_raw = {0};
  uint8_t current_raw_byte = 0;
  std::array<uint8_t, IZAR_LENGH> buffer = {0};

  OsTime debugtime;
};

#endif