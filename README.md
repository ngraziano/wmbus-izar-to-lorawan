# Lora proxy for izar wmbus

This project listen to izar frame every 5 minute and transmit flags and current meter index by lorawan.
When you meter is far away from your network, it allow to use lorawan.

The SX1276 is used both for wmbus (in FSK mode) and for lorawan, it's allow to have a simple circuit 
with just a SX1276 connected to a microcontroller.

Only work with IZAR frame (length harcoded) but can be adapted for other wmbus frame

Tested with Arduino Pro Mini and RFM95 on EU868 frequencies.
*Warning* : Not standart bootloader **must** be installed to handle watchdog and low voltage

## Usage

Work with platformio.

In ``src`` directory create a file named ``lorakeys.h`` wich contain the keys declared in network (for exemple <https://www.thethingsnetwork.org>)

Exemple of file:

```cpp
// Application in string format.
// For TTN issued EUIs the first bytes should be 70B3D5
constexpr char const appEui[] = "70B3D5XXXXXXXXXX";

// Device EUI in string format.
constexpr char const devEui[] = "XXXXXXXXXXXXXXXX";
// Application key in string format.
constexpr char const appKey[] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";

// The id of the meter (you can find it by watching the start of decoded payload (from byte 4))
constexpr std::array<uint8_t, 6> my_meter = {0xAA, 0xAA, 0xAA,
                                               0xAA, 0x98, 0x01};

```

Calibrate deepsleep duration (see below)

Int 1 / Pin 3 is use to wake a with a button linked to ground.

## Calibration of deep sleep

During start there is a test of deepsleep time.
You need to calibrate it.
Use a terminal which print the time the message are receive (YAT for example) and mesure time between message `Start Test sleep time.` and `End Test sleep time.` divide this time by the time in `Test Time should be :` message and ajust `sleepAdj` acordingly.
