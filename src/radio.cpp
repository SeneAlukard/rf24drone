#include "../include/radio.hpp"
#include <cstdint>
#include <cstring>
#include <iostream>

RadioInterface::RadioInterface(uint8_t cePin, uint8_t csnPin)
    : tx_radio(std::make_unique<RF24>(cePin, csnPin)) {}

RadioInterface::RadioInterface(uint8_t txCePin, uint8_t txCsnPin,
                               uint8_t rxCePin, uint8_t rxCsnPin)
    : tx_radio(std::make_unique<RF24>(txCePin, txCsnPin)),
      rx_radio(std::make_unique<RF24>(rxCePin, rxCsnPin)), full_duplex(true) {}

RadioInterface::RadioInterface(uint8_t txCePin, uint8_t txCsnPin,
                               uint8_t txSpiPort, uint8_t rxCePin,
                               uint8_t rxCsnPin, uint8_t rxSpiPort)
    : tx_radio(std::make_unique<RF24>(txCePin, txCsnPin, txSpiPort)),
      rx_radio(std::make_unique<RF24>(rxCePin, rxCsnPin, rxSpiPort)),
      full_duplex(true) {}

bool RadioInterface::begin() {
  if (!tx_radio->begin())
    return false;
  tx_radio->setPALevel(RF24_PA_MAX);
  if (full_duplex && rx_radio) {
    if (!rx_radio->begin())
      return false;
    rx_radio->setPALevel(RF24_PA_MAX);
    rx_radio->startListening();
  } else {
    tx_radio->stopListening();
  }
  return true;
}

void RadioInterface::setAddress(uint64_t tx, uint64_t rx) {
  tx_address = tx;
  rx_address = rx;
  tx_radio->openWritingPipe(tx_address);
  if (full_duplex && rx_radio)
    rx_radio->openReadingPipe(1, rx_address);
  else
    tx_radio->openReadingPipe(1, rx_address);
}

void RadioInterface::openListeningPipe(uint8_t pipe, uint64_t address) {
  if (full_duplex && rx_radio)
    rx_radio->openReadingPipe(pipe, address);
  else
    tx_radio->openReadingPipe(pipe, address);
}

void RadioInterface::configure(uint8_t channel, RadioDataRate datarate) {
  auto configureRadio = [&](RF24 *r) {
    r->setChannel(channel);
    switch (datarate) {
    case RadioDataRate::LOW_RATE:
      r->setDataRate(RF24_250KBPS);
      break;
    case RadioDataRate::MEDIUM_RATE:
      r->setDataRate(RF24_1MBPS);
      break;
    case RadioDataRate::HIGH_RATE:
      r->setDataRate(RF24_2MBPS);
      break;
    }
    r->setAutoAck(true);
    r->enableDynamicPayloads();
    r->enableAckPayload();
  };

  configureRadio(tx_radio.get());
  if (full_duplex && rx_radio) {
    configureRadio(rx_radio.get());
    rx_radio->startListening();
  } else {
    tx_radio->startListening();
  }
}

bool RadioInterface::send(const void *data, size_t size) {
  if (full_duplex && rx_radio) {
    return tx_radio->write(data, static_cast<uint8_t>(size));
  }
  tx_radio->stopListening();
  bool success = tx_radio->write(data, static_cast<uint8_t>(size));
  tx_radio->startListening();
  return success;
}

bool RadioInterface::receive(void *data, size_t size, bool peekOnly) {
  RF24 *rx = (full_duplex && rx_radio) ? rx_radio.get() : tx_radio.get();

  if (peekOnly) {
    if (cached_packet) {
      std::memcpy(data, cached_packet->data(), size);
      return true;
    }

    if (!rx->available())
      return false;

    cached_packet.emplace();
    rx->read(cached_packet->data(),
             static_cast<uint8_t>(cached_packet->size()));
    std::memcpy(data, cached_packet->data(), size);
    return true;
  }

  if (cached_packet) {
    std::memcpy(data, cached_packet->data(), size);
    cached_packet.reset();
    return true;
  }

  if (!rx->available())
    return false;

  std::array<uint8_t, 32> buffer{};
  rx->read(buffer.data(), buffer.size());
  std::memcpy(data, buffer.data(), size);
  cached_packet.reset();
  return true;
}

bool RadioInterface::testRPD() {
  RF24 *rx = (full_duplex && rx_radio) ? rx_radio.get() : tx_radio.get();
  return rx->testRPD();
}

uint8_t RadioInterface::getARC() { return tx_radio->getARC(); }
