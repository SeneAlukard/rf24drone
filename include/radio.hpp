#pragma once

#include "packets.hpp"
#include <RF24.h>
#include <array>
#include <cstdint>
#include <optional>

enum class RadioDataRate {
  LOW_RATE,
  MEDIUM_RATE,
  HIGH_RATE,
};

class RadioInterface {
public:
  RadioInterface(uint8_t cePin, uint8_t csnPin); // for default SPI BUS
  RadioInterface(uint8_t cePin, uint8_t csnPin,
                 uint8_t spiPort); // for custom SPI BUS

  bool begin();
  void setAddress(uint64_t tx, uint64_t rx);
  void openListeningPipe(uint8_t pipe, uint64_t address);
  void configure(uint8_t channel = 90,
                 RadioDataRate datarate = RadioDataRate::MEDIUM_RATE);

  bool send(const void *data, size_t size);
  bool receive(void *data, size_t size, bool peekOnly = false);

private:
  RF24 radio;
  uint64_t tx_address = 0;
  uint64_t rx_address = 0;
  std::optional<std::array<uint8_t, 32>> peek_buffer;
};
