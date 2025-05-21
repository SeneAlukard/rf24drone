#include "radio.hpp"

RF24 radio(22, 0); // CE_PIN, SPI_CEO

void sendCommandAndReceive(uint8_t ID, const uint8_t *drone_addr) {
  radio.stopListening();
  radio.startListening();

  CommandPacket cmd = {ID, "SEND_STATUS"};
  bool success = radio.write(&cmd, sizeof(cmd));

  if (!success) {
    std::cout << "[!] Failed to send commands to drone:  "
              << static_cast<int>(ID) << std::endl;
    return;
  }

  radio.openReadingPipe(1, drone_addr);
  radio.startListening();

  size_t start = millis();
  while (!radio.available() && millis() - start < 200)
    ;

  if (radio.available()) {
    TelemetryPacket data;
    radio.read(&data, sizeof(data));
    radio.stopListening();
    usleep(5000);
  }
}

void setupRFCommunication() {
  radio.setPALevel(RF24_PA_LOW);
  radio.setChannel(108);
  radio.setDataRate(RF24_250KBPS);
  radio.setRetries(5, 15);
  radio.setAutoAck(true);
  radio.enableDynamicPayloads();
  radio.setCRCLength(RF24_CRC_16);
  radio.openReadingPipe(1, (const uint8_t *)"DRN01");
  radio.openWritingPipe((const uint8_t *)"DRN01");
  radio.startListening();

  std::cout << "📡 GBS started. Polling drones..." << std::endl;
}
