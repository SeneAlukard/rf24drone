#include "drone.hpp"
#include "mpu6050.hpp"
#include "packets.hpp"
#include "radio.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <thread>
#include <vector>

#define TX_CE_PIN 27
#define TX_CSN_PIN 0
#define RX_CE_PIN 22
#define RX_CSN_PIN 10
static constexpr uint64_t BASE_TX = 0xF0F0F0F0D2ULL;
static constexpr uint64_t BASE_RX = 0xF0F0F0F0E1ULL;

// Basit lider döngüsü
static void leaderLoop(RadioInterface &radio, Drone &drone,
                       const std::vector<DroneIdType> &swarm) {
  size_t idx = 0;
  while (drone.isLeader()) {
    // Yer istasyonu ile konuşmak için kanalı değiştir
    radio.configure(1, RadioDataRate::MEDIUM_RATE);
    radio.setAddress(BASE_TX, BASE_RX);

    std::cout << drone_channel << gbs_channel << std::endl;

    PermissionToSendPacket perm{};
    perm.target_drone_id = 0; // 0 -> GBS
    perm.timestamp = static_cast<uint32_t>(std::time(nullptr));
    radio.send(&perm, sizeof(perm));

    // Kısa süre komut bekle
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start <
           std::chrono::milliseconds(300)) {
      PacketType peek;
      if (radio.receive(&peek, sizeof(peek), true) &&
          peek == PacketType::COMMAND) {
        CommandPacket cmd{};
        if (radio.receive(&cmd, sizeof(cmd))) {
          if (std::strcmp(cmd.command, "no_need") == 0) {
            // no action needed, simply noted
          }
        }
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Drone kanalı
    radio.configure(1, RadioDataRate::MEDIUM_RATE);
    radio.setAddress(BASE_TX, BASE_RX);

    if (!swarm.empty()) {
      DroneIdType target = swarm[idx];
      PermissionToSendPacket p{};
      p.target_drone_id = target;
      p.timestamp = static_cast<uint32_t>(std::time(nullptr));
      radio.send(&p, sizeof(p));

      auto waitStart = std::chrono::steady_clock::now();
      while (std::chrono::steady_clock::now() - waitStart <
             std::chrono::milliseconds(300)) {
        PacketType t;
        if (radio.receive(&t, sizeof(t), true)) {
          drone.handleIncoming();
          break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }

      idx = (idx + 1) % swarm.size();
    }

    if (drone.hasRoleChanged()) {
      drone.clearRoleChanged();
      break;
    }
  }
}

static void followerLoop(RadioInterface &radio, Drone &drone, Mpu6050 *sensor) {
  auto last_leader = std::chrono::steady_clock::now();
  auto last_telemetry = std::chrono::steady_clock::now();

  while (!drone.isLeader()) {
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
      int16_t ax = 0, ay = 0, az = 0;
      int16_t gx = 0, gy = 0, gz = 0;
      bool ok = false;
      if (sensor) {
        ok = sensor->readAcceleration(ax, ay, az) &&
             sensor->readGyro(gx, gy, gz);
      }
      if (!ok) {
        ax = static_cast<int16_t>(rand() % 100);
        ay = static_cast<int16_t>(rand() % 100);
        az = static_cast<int16_t>(rand() % 100);
        gx = static_cast<int16_t>(rand() % 50);
        gy = static_cast<int16_t>(rand() % 50);
        gz = static_cast<int16_t>(rand() % 50);
      }
      drone.updateSensors(ax, ay, az, gx, gy, gz, 120.0f, 3.7f);
      drone.sendTelemetry();
      last_telemetry = now;
    }

    if (now - last_leader > std::chrono::seconds(5)) {
      LeaderRequestPacket req{};
      req.drone_id = drone.getNetworkId().value_or(drone.getTempId());
      req.timestamp = static_cast<uint32_t>(std::time(nullptr));
      radio.send(&req, sizeof(req));
      last_leader = now;
    }

    if (drone.hasRoleChanged()) {
      drone.clearRoleChanged();
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}

int main(int argc, char **argv) {
  bool leader_mode = false;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--leader") == 0)
      leader_mode = true;
  }

  RadioInterface radio(TX_CE_PIN, TX_CSN_PIN, RX_CE_PIN, RX_CSN_PIN);

  Mpu6050 sensor;
  if (!sensor.init()) {
    std::cerr << "MPU6050 başlatılamadı, rasgele veriler kullanılacak\n";
  }

  if (!radio.begin()) {
    std::cerr << "Radio başlatılamadı!\n";
    return 1;
  }

  // --- Katılma Aşaması ---
  radio.configure(1, RadioDataRate::MEDIUM_RATE);
  radio.setAddress(BASE_TX, BASE_RX);

  Drone drone(radio, leader_mode);
  JoinRequestPacket join{};
  join.timestamp = static_cast<uint32_t>(std::time(nullptr));
  join.temp_id = drone.getTempId();
  std::strncpy(join.requested_name, drone.getName().c_str(),
               MAX_NODE_NAME_LENGTH - 1);
  join.requested_name[MAX_NODE_NAME_LENGTH - 1] = '\0';

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
            << " Lider: " << static_cast<int>(leader_id)
            << " Kanal: 1" << std::endl;

  drone.setCurrentLeaderId(leader_id);
  drone.setLeaderStatus(leader_id == resp.assigned_id);

  // --- Operasyon Aşaması ---
  radio.configure(1, RadioDataRate::MEDIUM_RATE);
  radio.setAddress(BASE_TX, BASE_RX);

  std::vector<DroneIdType> swarm{1, 2, 3};
  swarm.erase(std::remove(swarm.begin(), swarm.end(),
                          drone.getNetworkId().value_or(drone.getTempId())),
              swarm.end());

  while (true) {
    if (drone.isLeader())
      leaderLoop(radio, drone, swarm);
    else
      followerLoop(radio, drone, &sensor);
  }

  return 0;
}
