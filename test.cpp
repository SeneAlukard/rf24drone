#include <RF24/RF24.h>
#include <cstdint>
#include <cstdio>
#include <unistd.h>

// CE = GPIO27, CSN = GPIO10
RF24 radio(27, 10);

const uint8_t address[6] = {'1', 'N', 'o', 'd', 'e', '\0'};

int main() {
  if (!radio.begin()) {
    std::printf("NRF24 initialization failed\n");
    return 1;
  }

  radio.setPALevel(RF24_PA_LOW);
  radio.openReadingPipe(1, address);
  radio.startListening();

  char buffer[32] = {0};
  while (true) {
    if (radio.available()) {
      radio.read(buffer, sizeof(buffer));
      std::printf("Received: %s\n", buffer);
      std::fflush(stdout);
    }
    usleep(100000);
  }
  return 0;
}
