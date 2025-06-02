// transmitter.cpp
#include <RF24/RF24.h>
#include <iostream>
#include <string>
#include <unistd.h>

// CE pin = BCM22, CSN = BCM0 (SPI CE0)
// adjust if you wire differently
RF24 radio(22, 0);

// Unique 64-bit address for the pipe
const uint64_t PIPE_ADDRESS = 0xE8E8F0F0E1LL;

int main() {
  std::string msg;

  if (!radio.begin()) {
    std::cerr << "nRF24 init failed\n";
    return 1;
  }

  radio.setRetries(5, 15);  // delay, count
  radio.setPayloadSize(32); // max 32 bytes
  radio.openWritingPipe(PIPE_ADDRESS);
  radio.stopListening(); // set as TX

  std::cout << "Transmitter ready. Type messages:\n";
  while (true) {
    std::getline(std::cin, msg);
    if (msg.empty())
      continue;
    if (msg == "exit")
      break;
    // pad or truncate to 32 bytes
    char data[32] = {0};
    strncpy(data, msg.c_str(), sizeof(data) - 1);
    bool ok = radio.write(&data, sizeof(data));
    if (ok)
      std::cout << "[OK] Sent: " << msg << "\n";
    else
      std::cout << "[FAIL] Send error\n";
    usleep(200000); // 200 ms between sends
  }
  return 0;
}
