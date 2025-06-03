#pragma once

#include "packets.hpp"
#include <RF24/RF24.h>

enum class RadioDataRate {
  LOW_RATE,
  MEDIUM_RATE,
  HIGH_RATE,
};

class RadioInterface {
public:
  RadioInterface(uint8_t cePin, uint8_t csnPin);

  bool begin();

  void setAddress(uint64_t tx_address, uint64_t rx_address);

  bool send(const void *data, size_t size);

  bool receive(void *data, size_t size);

  void openListeningPipe(uint8_t pipe, uint64_t address);

  void configure(uint8_t channel = 90,
                 RadioDataRate datarate = RadioDataRate::MEDIUM_RATE);

private:
  RF24 radio;
  uint64_t tx_address = 0;
  uint64_t rx_address = 0;
};
