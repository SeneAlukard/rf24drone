#pragma once

#include "packets.hpp"
#include <RF24.h>
#include <array>
#include <cstdint>
#include <optional>
#include <memory>

enum class RadioDataRate {
  LOW_RATE,
  MEDIUM_RATE,
  HIGH_RATE,
};

class RadioInterface {
public:
  // Single transceiver constructor
  RadioInterface(uint8_t cePin, uint8_t csnPin);

  // Full duplex constructors (separate TX and RX modules)
  RadioInterface(uint8_t txCePin, uint8_t txCsnPin, uint8_t rxCePin,
                 uint8_t rxCsnPin);
  RadioInterface(uint8_t txCePin, uint8_t txCsnPin, uint8_t txSpiPort,
                 uint8_t rxCePin, uint8_t rxCsnPin, uint8_t rxSpiPort);

  bool begin();
  void setAddress(uint64_t tx, uint64_t rx);
  void openListeningPipe(uint8_t pipe, uint64_t address);
  void configure(uint8_t channel = 90,
                 RadioDataRate datarate = RadioDataRate::MEDIUM_RATE);

  bool send(const void *data, size_t size);
  bool receive(void *data, size_t size, bool peekOnly = false);

  bool testRPD();
  uint8_t getARC();

private:
  std::unique_ptr<RF24> tx_radio;
  std::unique_ptr<RF24> rx_radio; // if null, single transceiver mode
  bool full_duplex = false;
  uint64_t tx_address = 0;
  uint64_t rx_address = 0;
  // Holds the latest packet when it was peeked so that it can be
  // retrieved again on the next receive call.
  std::optional<std::array<uint8_t, 32>> cached_packet;
};
