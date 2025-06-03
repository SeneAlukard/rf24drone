#include "../include/radio.hpp"

RadioInterface::RadioInterface(uint8_t cePin, uint8_t csnPin)
    : radio(cePin, csnPin) {}

bool RadioInterface::begin() {
  if (!radio.begin())
    return false;
  radio.setRetries(5, 15);
  radio.setPALevel(RF24_PA_LOW);
  radio.setCRCLength(RF24_CRC_16);
  radio.setAutoAck(true);
  radio.enableDynamicPayloads();
  radio.startListening();
  return true;
}

static rf24_datarate_e toRf24Rate(RadioDataRate rate) {
  switch (rate) {
  case RadioDataRate::LOW_RATE:
    return RF24_250KBPS;
  case RadioDataRate::MEDIUM_RATE:
    return RF24_1MBPS;
  case RadioDataRate::HIGH_RATE:
    return RF24_2MBPS;
  }
  return RF24_1MBPS;
}

void RadioInterface::configure(uint8_t channel, RadioDataRate rate) {
  radio.setChannel(channel);
  radio.setDataRate(toRf24Rate(rate));
}

void RadioInterface::setAddress(uint64_t tx_address_, uint64_t rx_address_) {
  this->tx_address = tx_address_;
  this->rx_address = rx_address_;
  radio.openWritingPipe(tx_address);
  radio.openReadingPipe(1, rx_address);
}

bool RadioInterface::send(const void *data, size_t size) {
  if (size > 32)
    return false;
  radio.stopListening();
  bool success = radio.write(data, static_cast<uint8_t>(size));
  radio.startListening();
  return success;
}

bool RadioInterface::receive(void *buffer, size_t size) {
  if (radio.available()) {
    radio.read(buffer, static_cast<uint8_t>(size));
    return true;
  }
  return false;
}

void RadioInterface::openListeningPipe(uint8_t pipe, uint64_t address) {
  radio.openReadingPipe(pipe, address);
}
