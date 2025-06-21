#include "./include/radio.hpp"
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

// Pin definitions for two drones (adjust to your wiring)
#define DRONE_A_TX_CE 27
#define DRONE_A_TX_CSN 10
#define DRONE_A_RX_CE 22
#define DRONE_A_RX_CSN 0

#define DRONE_B_TX_CE 17
#define DRONE_B_TX_CSN 0
#define DRONE_B_RX_CE 23
#define DRONE_B_RX_CSN 1

static constexpr uint64_t ADDR_A_TX = 0xF0F0F0F0AAULL;
static constexpr uint64_t ADDR_B_TX = 0xF0F0F0F0BBULL;

void droneThread(RadioInterface &radio, const char *sendMsg) {
  char buffer[32]{};
  for (int i = 0; i < 3; ++i) {
    radio.send(sendMsg, std::strlen(sendMsg) + 1);
    if (radio.receive(buffer, sizeof(buffer))) {
      std::cout << sendMsg << " received: " << buffer << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}

int main() {
  RadioInterface radioA(DRONE_A_TX_CE, DRONE_A_TX_CSN, DRONE_A_RX_CE,
                        DRONE_A_RX_CSN);
  RadioInterface radioB(DRONE_B_TX_CE, DRONE_B_TX_CSN, DRONE_B_RX_CE,
                        DRONE_B_RX_CSN);

  if (!radioA.begin() || !radioB.begin()) {
    std::cerr << "Failed to initialize radios" << std::endl;
    return 1;
  }

  radioA.configure(90, RadioDataRate::MEDIUM_RATE);
  radioB.configure(90, RadioDataRate::MEDIUM_RATE);
  radioA.setAddress(ADDR_A_TX, ADDR_B_TX);
  radioB.setAddress(ADDR_B_TX, ADDR_A_TX);

  std::thread ta(droneThread, std::ref(radioA), "DroneA");
  std::thread tb(droneThread, std::ref(radioB), "DroneB");

  ta.join();
  tb.join();

  return 0;
}
