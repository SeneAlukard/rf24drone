#pragma once

#include <cstdint>
#include <iostream>

class Drone {
private:
  int8_t RSSI;

public:
  explicit Drone(int8_t RSSI);

  void printRSSI const();
};
