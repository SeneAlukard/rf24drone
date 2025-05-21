#include "drone.hpp"
#include <cstdint>
#include <cwchar>
#include <ostream>

uint16_t Drone::counter = 0;

Drone::Drone(int8_t RSSI, uint32_t SOCKET, bool leaderStatus,
             const std::string &name)
    : RSSI(RSSI), SOCKET(SOCKET), leaderStatus(leaderStatus), name(name) {
  ID = ++counter;
}

uint16_t Drone::getID() const { return ID; }
int8_t Drone::getRSSI() const { return RSSI; }
uint32_t Drone::getSOCKET() const { return SOCKET; }
bool Drone::getleaderStatus() const { return leaderStatus; }
std::string Drone::getname() const { return name; }

// ---- SET FN PART ---- //

void Drone::setRSSI(int8_t RSSI) { this->RSSI = RSSI; }
void Drone::setleaderStatus(bool leaderStatus) {
  this->leaderStatus = leaderStatus;
}
void Drone::setSOCKET(uint32_t SOCKET) { this->SOCKET = SOCKET; }

// ---- PRINT FUNCTIONS PART ---- //

void Drone::printname() const { std::cout << "Name: " << name << std::endl; }
void Drone::printID() const { std::cout << "ID: " << ID << std::endl; }
void Drone::printRSSI() const { std::cout << "RSSI: " << RSSI << std::endl; }
void Drone::printSOCKET() const {
  std::cout << "SOCKET: " << SOCKET << std::endl;
}
void Drone::printleaderStatus() const {
  std::cout << "leaderStatus" << (leaderStatus ? "True" : "False") << std::endl;
}
void Drone::printDroneInfo() const {
  std::cout << "\nDrone info: \n    ID: " << ID << "\n    Name: " << name
            << "\n    RSSI : " << static_cast<int>(RSSI)
            << "\n    SOCKET: " << SOCKET
            << "\n    leaderStatus: " << (leaderStatus ? "true" : "false")
            << "\n"
            << std::endl;
}
