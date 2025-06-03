#include "drone.hpp"
#include "packets.hpp"
#include "radio.hpp"
#include <ctime>
#include <iostream>
#include <thread>
#include <unistd.h>
#include <vector>

#define CE_PIN 22
#define CSN_PIN 0

int main() {
  RadioInterface radio(CE_PIN, CSN_PIN);

  if (!radio.begin()) {
    std::cerr << "Radio başlatılamadı!\n";
    return 1;
  }

  radio.configure(85, RadioDataRate::MEDIUM_RATE);
  radio.setAddress(0xF0F0F0F0D2LL, 0xF0F0F0F0E1LL);

  bool is_leader = false;
  std::cout << "Bu cihaz lider mi? (1 = evet, 0 = hayır): ";
  std::cin >> is_leader;

  Drone drone(radio, is_leader);

  if (is_leader) {
    radio.setAddress(0xF0F0F0F0D2LL, 0xF0F0F0F0E1LL);
    std::vector<DroneIdType> drone_ids = {1, 2, 3}; // örnek id’ler
    size_t current = 0;

    while (true) {
      PermissionToSendPacket packet{
          .type = PacketType::PERMISSION_TO_SEND,
          .target_drone_id = drone_ids[current],
          .timestamp = static_cast<uint32_t>(std::time(nullptr))};

      bool sent = radio.send(&packet, sizeof(packet));
      std::cout << "→ Drone " << static_cast<int>(packet.target_drone_id)
                << " için izin gönderildi: " << (sent ? "EVET" : "HAYIR")
                << std::endl;

      current = (current + 1) % drone_ids.size();
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  } else {
    while (true) {
      drone.handleIncoming(); // izin ya da komut varsa işlenir

      // Simüle sensör verisi
      drone.updateSensors(static_cast<int16_t>(rand() % 100),
                          static_cast<int16_t>(rand() % 100),
                          static_cast<int16_t>(rand() % 100),
                          static_cast<int16_t>(rand() % 50),
                          static_cast<int16_t>(rand() % 50),
                          static_cast<int16_t>(rand() % 50), 120.0f, 3.7f);

      drone.sendTelemetry(); // sadece izin geldiyse gönderir

      HeartbeatPacket hb{.type = PacketType::HEARTBEAT,
                         .source_drone_id = drone.getTempId(),
                         .timestamp =
                             static_cast<uint32_t>(std::time(nullptr))};
      radio.send(&hb, sizeof(hb));

      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  }

  return 0;
}
