#include <RF24/RF24.h>
#include <unistd.h>

// CE = 27, CSN = 0 (spidev1.0)
RF24 radio(27, 10);

const byte address[6] = "1Node";

int main() {
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.openReadingPipe(1, address);
  radio.startListening();

  char text[32] = {0};
  while (1) {
    if (radio.available()) {
      radio.read(&text, sizeof(text));
      printf("Received: %s\n", text);
    }
    usleep(100000); // 100ms
  }
}
