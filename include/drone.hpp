#pragma once
#include <cstdint>
#include <iostream>
#include <string>

class Drone {
private:
  uint16_t ID;
  int8_t RSSI;
  uint32_t SOCKET;
  bool leaderStatus;
  std::string name;

  static uint16_t counter;

public:
  Drone(int8_t RSSI = 0, uint32_t SOCKET = 0, bool leaderStatus = false,
        const std::string &name = "unknown");

  uint16_t getID() const;
  int8_t getRSSI() const;
  uint32_t getSOCKET() const;
  bool getleaderStatus() const;
  std::string getname() const;

  void printname() const;
  void printID() const;
  void printRSSI() const;
  void printSOCKET() const;
  void printleaderStatus() const;
  void printDroneInfo() const;

  void setRSSI(int8_t RSSI);
  void setSOCKET(uint32_t SOCKET);
  void setleaderStatus(bool leaderStatus);
};
