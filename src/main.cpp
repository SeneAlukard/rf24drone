#include <RF24/RF24.h>
#include <cstdint>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

// ----------- USER DEFINED INCLUDES ----------- //
#include "../include/drone.hpp"
#include "../include/packets.hpp"
#include "../include/printSet.hpp"
#include "../include/radio.hpp"

// ---------------  USER DEFINED CONSTANTS -------------------- //
#define int8t_min -128
#define int8t_max 127

#define CE_PIN 22
#define SPI_CEO 0

// -------------- GLOBAL VARIABLES --------------------//
//
const uint8_t DRONE1_ADDR[5] = {'D', 'R', 'N', '0', '1'};
const uint8_t DRONE2_ADDR[5] = {'D', 'R', 'N', '0', '2'};
const uint8_t DRONE3_ADDR[5] = {'D', 'R', 'N', '0', '3'};
const uint8_t GBS_ADDR[5] = {'G', 'B', 'S', '0', '0'};

bool debug = false;

static uint16_t droneCounter = 0;
static uint16_t leaderID = 0;

bool leaderAlgorithm(std::vector<Drone> &droneList) {
  int8_t maxRSSI = int8t_min;
  for (const auto &x : droneList)
    if ((x.getRSSI() - 10) > maxRSSI) {
      leaderID = x.getID();
      maxRSSI = x.getRSSI();
    }

  if (debug) {
    std::cout << "\n[DEBUG] Max RSSI selected : " << leaderID
              << "\n[DEBUG] New Leader ID selected : "
              << static_cast<int>(maxRSSI) << std::endl;
  }

  for (size_t i = 0; i < droneList.size(); i++)
    if (leaderID == droneList[i].getID())
      droneList[i].setleaderStatus(true);
    else
      droneList[i].setleaderStatus(false);

  if (debug) {
    uint8_t leaderCounter = 0;
    for (const auto &x : droneList)
      if (x.getleaderStatus())
        leaderCounter++;
    if (leaderCounter > 1)
      std::cerr << "More than 1 leader drone!!" << std::endl;
  }

  return true;
}

void vectorPrint(std::vector<Drone> vectorList) {

  std::vector<std::string> droneOutputList;

  for (const auto &drone : vectorList) {
    std::string output = captureOutput([&]() { drone.printDroneInfo(); });
    droneOutputList.push_back(output);
  }

  printSideBySide(droneOutputList);
}

// ------------ MAIN FUNCTION ------------ //

int main(int argc, char *argv[]) {
  debug = true;

  if (argc == 1)
    std::cout << *argv << "\n";
  else {
    std::cerr << "Inccorrect usage: \nUse like this: ./rfserver <no argument>"
              << std::endl;
    return 1;
  }

  std::cout << "Input argument count: " << argc << "\n";

  Drone a(10, 10, true, "xKea");
  Drone b(30, 10, false, "xKenshi");
  Drone c(30, 10, false, "xXx");
  Drone d(40, 10, false, "atilim");

  std::vector<Drone> x;
  x.push_back(a);
  x.push_back(b);
  x.push_back(c);
  x.push_back(d);

  vectorPrint(x);

  leaderAlgorithm(x);

  vectorPrint(x);

  std::cout << "[DEBUG] Drone count before exiting the code: " << droneCounter
            << std::endl;

  return 0;
}
