#include "radio1276FSK.h"

#include <algorithm>
#include <hal/print_debug.h>
#include <lmic/lmic_table.h>
#include <lmic/oslmic.h>
#include <lmic/radio_sx1276.h>
#include <stdio.h>

#include "izar.h"
#include "mbus_packet.h"
#include "powersave.h"

RadioSx1276FSK::RadioSx1276FSK(lmic_pinmap const &pins, const std::array<uint8_t, 6> &meter_id)
    : meter_id(meter_id), hal(pins) {}

namespace {
constexpr uint8_t RegFifo = 0x00;   // common
constexpr uint8_t RegOpMode = 0x01; // common
constexpr uint8_t RegBitrateMsb = 0x02;
constexpr uint8_t RegBitrateLsb = 0x03;
constexpr uint8_t RegFdevMsb = 0x04;
constexpr uint8_t RegFdevLsb = 0x05;

constexpr uint8_t RegFrfMsb = 0x06;   // common
constexpr uint8_t RegFrfMid = 0x07;   // common
constexpr uint8_t RegFrfLsb = 0x08;   // common
constexpr uint8_t RegPaConfig = 0x09; // common
constexpr uint8_t RegPaRamp = 0x0A;   // common
constexpr uint8_t RegOcp = 0x0B;      // common
constexpr uint8_t RegLna = 0x0C;      // common
constexpr uint8_t RegRxConfig = 0x0D;
constexpr uint8_t RegRssiConfig = 0x0E;
constexpr uint8_t RegRxBw = 0x12;
constexpr uint8_t RegAfcBw = 0x13;
constexpr uint8_t RegAfcFei = 0x1A;
constexpr uint8_t RegPreambleDetect = 0x1F;
constexpr uint8_t RegOsc = 0x24;
constexpr uint8_t RegPreambleMsb = 0x25;
constexpr uint8_t RegPreambleLsb = 0x26;
constexpr uint8_t RegSyncConfig = 0x27;
constexpr uint8_t RegSyncValue1 = 0x28;
constexpr uint8_t RegSyncValue2 = 0x29;
constexpr uint8_t RegSyncValue3 = 0x2A;
constexpr uint8_t RegSyncValue4 = 0x2B;
constexpr uint8_t RegPacketConfig1 = 0x30;
constexpr uint8_t RegPacketConfig2 = 0x31;
constexpr uint8_t RegPayloadLength = 0x32;
constexpr uint8_t RegFifoThresh = 0x35;
constexpr uint8_t RegSeqConfig1 = 0x36;
constexpr uint8_t RegSeqConfig2 = 0x37;
constexpr uint8_t RegImageCal = 0x3B;
constexpr uint8_t RegIrqFlags1 = 0x3E;
constexpr uint8_t RegIrqFlags2 = 0x3F;
constexpr uint8_t RegDioMapping1 = 0x40;
constexpr uint8_t RegDioMapping2 = 0x41;

constexpr uint8_t OPMODE_MASK = 0x07;
constexpr uint8_t OPMODE_FSK = 0x00;

constexpr uint8_t OPMODE_SLEEP = 0x00;
constexpr uint8_t OPMODE_STANDBY = 0x01;
constexpr uint8_t OPMODE_FSTX = 0x02;
constexpr uint8_t OPMODE_TX = 0x03;
constexpr uint8_t OPMODE_FSRX = 0x04;
constexpr uint8_t OPMODE_RX = 0x05;
constexpr uint8_t OPMODE_RX_SINGLE = 0x06;
constexpr uint8_t OPMODE_CAD = 0x07;

constexpr uint8_t IrqFifoFull = 0x80;
constexpr uint8_t IrqFifoEmpty = 0x40;
constexpr uint8_t IrqFifoLevel = 0x20;
constexpr uint8_t IrqFifoOverrun = 0x10;
constexpr uint8_t IrqPacketSent = 0x08;
constexpr uint8_t IrqPayloadReady = 0x04;
constexpr uint8_t IrqCrcOk = 0x02;
constexpr uint8_t IrqRssi = 0x01;

const uint32_t xtal_freq = 32000000;

// Param
constexpr uint32_t t1_freq = 868950000;
constexpr uint64_t const frf = ((uint64_t)t1_freq << 19) / xtal_freq;

constexpr uint32_t t1_deviation = 50000;
constexpr uint16_t fdev = ((uint64_t)t1_deviation << 19) / xtal_freq;

constexpr uint32_t t1_datarate = 100000;
constexpr uint32_t dt = xtal_freq / t1_datarate;

constexpr uint32_t preambleLen = 3;
constexpr uint32_t syncWord = 0x5555543DULL;
constexpr uint8_t fifoThreshold = 15;

CONST_TABLE(uint16_t, FSK_INIT_CMD)
[] = {
    RegSet(RegLna, 0x23).raw(),
    // RestartRxWithPLLClock, AfcAutoOn, AGC
    // auto on, PreambleDetect, AGC & AFC
    RegSet(RegRxConfig, 0x1E).raw(),
    // RSSI Offset, RSSI smoothing using 8 samples
    RegSet(RegRssiConfig, 0xD2).raw(),
    // AfcAutoClearOn
    RegSet(RegAfcFei, 0x01).raw(),
    // PreambleDetectorOn, PreambleDetectorSize = 3 bytes, 4
    // chip errros per bit tolerated
    RegSet(RegPreambleDetect, 0xAA).raw(),
    // ClkOut OFF
    RegSet(RegOsc, 0x07).raw(),
    // AutoRestartRxMod = wait for PLL to lock, PreamblePolarity =
    // 0x55, Sync on, Size of the Sync Word = SyncSize + 1 = 2
    RegSet(RegSyncConfig, 0xb3).raw(),

    // Sync Word
    RegSet(RegSyncValue1, (uint8_t)(syncWord >> 24)).raw(),
    RegSet(RegSyncValue2, (uint8_t)(syncWord >> 16)).raw(),
    RegSet(RegSyncValue3, (uint8_t)(syncWord >> 8)).raw(),
    RegSet(RegSyncValue4, (uint8_t)(syncWord >> 0)).raw(),

    RegSet(RegFifoThresh, fifoThreshold).raw(),
    // Temperature change threshold = 10°C
    RegSet(RegImageCal, 0x02).raw(),
    // freq
    RegSet(RegFrfMsb, (uint8_t)(frf >> 16)).raw(),
    RegSet(RegFrfMid, (uint8_t)(frf >> 8)).raw(),
    RegSet(RegFrfLsb, (uint8_t)(frf >> 0)).raw(),

    // deviation
    // FSTEP= 32,000,000 / 2^19 = 61.03515625 Hz
    // FDEV = 50,000 / 61.03515625 = 819.2
    RegSet(RegFdevMsb, (uint8_t)(fdev >> 8)).raw(),
    RegSet(RegFdevLsb, (uint8_t)(fdev >> 0)).raw(),

    // datarate
    RegSet(RegBitrateMsb, (uint8_t)(dt >> 8)).raw(),
    RegSet(RegBitrateLsb, (uint8_t)(dt >> 0)).raw(),

    // bandwidth 2*t1_deviation + t1_datarate
    // 2*50_000 + 100_000 = 200_000
    // => register value 0x09
    RegSet(RegRxBw, 0x09).raw(),
    RegSet(RegAfcBw, 0x09).raw(),

    // preamble (not sure)
    RegSet(RegPreambleMsb, (uint8_t)((preambleLen >> 8) & 0xFF)).raw(),
    RegSet(RegPreambleLsb, (uint8_t)(preambleLen & 0xFF)).raw(),

    // payload length
    // limited to only one type of frame
    RegSet(RegPayloadLength, IZAR_LENGH_3OUTOF6).raw(),
    // config
    // addr filtering off, crc off, fixed length
    // packet mode
    RegSet(RegPacketConfig1, 0x00).raw(),
    RegSet(RegPacketConfig2, 0x40).raw(),

    // DIO mapping
    // DIO0=PayloadReady => Read Data
    // DIO1=FifoLevel    => Read Data
    // DIO2=FifoFull     => Not used
    // DIO3=FifoEmpty    => Not used
    // DIO4=Preamble     => Not used
    // DIO5=ModeReady    => Not used
    RegSet(RegDioMapping1, 0b00000000).raw(),
    RegSet(RegDioMapping2, 0b11000001).raw(),

    // transition
    // receive to low power
    // From Standby to Rx
    RegSet(RegSeqConfig2, 0x04).raw(),
}; // namespace

constexpr uint8_t NB_TX_INIT_CMD = sizeof(RESOLVE_TABLE(FSK_INIT_CMD)) / sizeof(RESOLVE_TABLE(FSK_INIT_CMD)[0]);

} // namespace

void RadioSx1276FSK::init() {
  PRINT_DEBUG(1, F("Config wmbus"));
  if ((hal.read_reg(RegOpMode) & 0xF0) != 0) {
    // need to go to sleep state if not in FSK mode
    hal.write_reg(RegOpMode, OPMODE_FSK | OPMODE_SLEEP);
  }

  hal.write_reg(RegOpMode, OPMODE_FSK | OPMODE_STANDBY);

  for (uint8_t i = 0; i < NB_TX_INIT_CMD; i++) {
    RegSet cmd{table_get_u2(RESOLVE_TABLE(FSK_INIT_CMD), i)};
    hal.write_reg(cmd.reg, cmd.val);
  }

  PRINT_DEBUG(1, F("Config done"));
}

void RadioSx1276FSK::handle_payload_ready() {
  // Read end of packet
  uint8_t remaining = IZAR_LENGH_3OUTOF6 - current_raw_byte;
  hal.read_buffer(RegFifo, buffer_raw.begin() + current_raw_byte, remaining);
  current_raw_byte += remaining;
  hal.write_reg(RegOpMode, OPMODE_STANDBY);
}

void RadioSx1276FSK::handle_fifo_level() {
  // Read partial FIFO
  uint8_t remaining = IZAR_LENGH_3OUTOF6 - current_raw_byte;
  uint8_t to_read = std::min(remaining, (uint8_t)(fifoThreshold - 1));
  hal.read_buffer(RegFifo, buffer_raw.begin() + current_raw_byte, to_read);
  current_raw_byte += to_read;
}

bool RadioSx1276FSK::listen_wmbus(std::array<uint8_t, 7> &result) {
  bool isFind = false;
  if (!listening) {
    PRINT_DEBUG(1, F("Start listen wmbus"));
    init();
    current_raw_byte = 0;

    // start rx
    hal.write_reg(RegOpMode, (hal.read_reg(RegOpMode) & ~OPMODE_MASK) | OPMODE_RX);
    listening = true;
  }

  if (hal.io_check1()) {
    handle_fifo_level();
  }

  if (hal.io_check0()) {
    handle_payload_ready();
  }

#if LMIC_DEBUG_LEVEL > 1
  if (os_getTime() - debugtime > OsDeltaTime::from_sec(5)) {
    debugtime = os_getTime();
    auto irq1 = hal.read_reg(RegIrqFlags1);
    auto irq2 = hal.read_reg(RegIrqFlags2);
    PRINT_DEBUG(1, F("State %02x, IRQ1 %02x, IRQ2 %02x, current_raw_byte %d"), hal.read_reg(RegOpMode), irq1, irq2,
                current_raw_byte);

    if (irq2 & IrqFifoOverrun) {
      hal.write_reg(RegIrqFlags2, IrqFifoOverrun);
      hal.write_reg(RegOpMode, (hal.read_reg(RegOpMode) & ~OPMODE_MASK) | OPMODE_STANDBY);
      PRINT_DEBUG(1, F("Fifo overrun"));

      hal.write_reg(RegOpMode, (hal.read_reg(RegOpMode) & ~OPMODE_MASK) | OPMODE_RX);
    }

    if (irq2 & IrqFifoFull) {
      hal.write_reg(RegOpMode, (hal.read_reg(RegOpMode) & ~OPMODE_MASK) | OPMODE_STANDBY);
      PRINT_DEBUG(1, F("Fifo full"));
      hal.write_reg(RegOpMode, (hal.read_reg(RegOpMode) & ~OPMODE_MASK) | OPMODE_RX);
    }
  }
#endif

  if (current_raw_byte == IZAR_LENGH_3OUTOF6) {
    PRINT_DEBUG(1, F("Payload read"));
    PRINT_DEBUG(1, F("Payload RAW: %02x %02x %02x %02x %02x %02x %02x %02x"), buffer_raw[0], buffer_raw[1],
                buffer_raw[2], buffer_raw[3], buffer_raw[4], buffer_raw[5], buffer_raw[6], buffer_raw[7]);

    auto decode_result = decodeRXBytesTmode(buffer_raw.begin(), buffer.begin(), IZAR_LENGH);

    PRINT_DEBUG(1, F("decode packet %d "), decode_result);
    PRINT_DEBUG(1, F("Payload: %02x %02x %02x %02x %02x %02x %02x %02x"), buffer[0], buffer[1], buffer[2], buffer[3],
                buffer[4], buffer[5], buffer[6], buffer[7]);
    PRINT_DEBUG(1, F("Payload: %02x %02x %02x %02x %02x %02x %02x %02x"), buffer[8], buffer[9], buffer[10], buffer[11],
                buffer[12], buffer[13], buffer[14], buffer[15]);

    if (decode_result == PacketDecodeResult::OK) {
      isFind = printAndExtractIZAR(buffer.begin(), buffer.size(), meter_id, result);
      if (LMIC_DEBUG_LEVEL > 0)
        printf("\n");
    }
    current_raw_byte = 0;
    listening = false;
  }

  return isFind;
}

void RadioSx1276FSK::stop_listen() {
  PRINT_DEBUG(1, F("Stopping listen WMBUS"));
  current_raw_byte = 0;
  listening = false;
  hal.write_reg(RegOpMode, OPMODE_SLEEP);
}
