#include "drone.hpp"
#include "packets.hpp"
#include "radio.hpp"
#include <chrono>
#include <cstring>
#include <ctime>
#include <iostream>
#include <thread>
#include <cstdlib>

#define CE_PIN 22
#define CSN_PIN 10

static constexpr uint64_t BASE_TX = 0xF0F0F0F0D2LL;
static constexpr uint64_t BASE_RX = 0xF0F0F0F0E1LL;

int main() {
  RadioInterface radio(CE_PIN, CSN_PIN);

  if (!radio.begin()) {
    std::cerr << "Radio başlatılamadı!\n";
    return 1;
  }

  // --- Katılma Aşaması ---
  radio.configure(0, RadioDataRate::MEDIUM_RATE);
  radio.setAddress(BASE_TX, BASE_RX);

  Drone drone(radio);
  JoinRequestPacket join{};
  join.timestamp = static_cast<uint32_t>(std::time(nullptr));
  join.temp_id = drone.getTempId();
  std::strncpy(join.requested_name, drone.getName().c_str(), MAX_NODE_NAME_LENGTH);
  radio.send(&join, sizeof(join));
  std::cout << "JoinRequest gönderildi, yanıt bekleniyor..." << std::endl;

  JoinResponsePacket resp{};
  while (true) {
    PacketType peek;
    if (radio.receive(&peek, sizeof(PacketType), true) &&
        peek == PacketType::JOIN_RESPONSE) {
      if (radio.receive(&resp, sizeof(resp))) {
        break;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  drone.setNetworkId(resp.assigned_id);
  DroneIdType leader_id = resp.current_leader_id;
  std::cout << "Ağ ID: " << static_cast<int>(resp.assigned_id)
            << " Lider: " << static_cast<int>(leader_id) << std::endl;

  // --- Operasyon Aşaması ---
  radio.configure(1, RadioDataRate::MEDIUM_RATE);
  radio.setAddress(BASE_TX, BASE_RX);

  auto last_leader = std::chrono::steady_clock::now();
  auto last_telemetry = std::chrono::steady_clock::now();

  while (true) {
    PacketType type;
    if (radio.receive(&type, sizeof(PacketType), true)) {
      if (type == PacketType::HEARTBEAT) {
        HeartbeatPacket hb;
        if (radio.receive(&hb, sizeof(hb))) {
          last_leader = std::chrono::steady_clock::now();
        }
      } else {
        drone.handleIncoming();
      }
    }

    auto now = std::chrono::steady_clock::now();
    if (now - last_telemetry > std::chrono::seconds(2)) {
      drone.updateSensors(static_cast<int16_t>(rand() % 100),
                          static_cast<int16_t>(rand() % 100),
                          static_cast<int16_t>(rand() % 100),
                          static_cast<int16_t>(rand() % 50),
                          static_cast<int16_t>(rand() % 50),
                          static_cast<int16_t>(rand() % 50), 120.0f, 3.7f);
      radio.configure(0, RadioDataRate::MEDIUM_RATE);
      drone.sendTelemetry();
      radio.configure(1, RadioDataRate::MEDIUM_RATE);
      last_telemetry = now;
    }

    if (now - last_leader > std::chrono::seconds(5)) {
      LeaderRequestPacket req{};
      req.drone_id = drone.getNetworkId().value_or(drone.getTempId());
      req.timestamp = static_cast<uint32_t>(std::time(nullptr));
      radio.configure(0, RadioDataRate::MEDIUM_RATE);
      radio.send(&req, sizeof(req));
      radio.configure(1, RadioDataRate::MEDIUM_RATE);
      last_leader = now;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  return 0;
}

