#include <RF24/RF24.h>
#include <cstdint>
#include <cstdio>
#include <unistd.h>

// CE = GPIO27, CSN = GPIO10
RF24 radio(22, 0);

const uint8_t address[6] = {'1', 'N', 'o', 'd', 'e', '\0'};

int main() {
  if (!radio.begin()) {
    std::printf("NRF24 init failed\n");
    return 1;
  }
  radio.setPALevel(RF24_PA_LOW);
  radio.openWritingPipe(address);
  radio.stopListening();

  const char payload[] = "Hello, NRF!";
  while (true) {
    radio.write(payload, sizeof(payload));
    sleep(1);
  }
  return 0;
}
