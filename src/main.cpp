// main.cpp - General communication loop (C++17 compatible)

#include "drone.hpp"
#include "radio.hpp"
#include <ctime>
#include <iostream>
#include <unistd.h>

#define CE_PIN 22
#define CSN_PIN 0
#define MAX_DRONES 5

int main() {
  RadioInterface radio(CE_PIN, CSN_PIN);

  if (!radio.begin()) {
    std::cerr << "Radio could not be started!\n";
    return 1;
  }

  radio.configure(85, RadioDataRate::MEDIUM_RATE);
  radio.setAddress(0xF0F0F0F0D2LL, 0xF0F0F0F0E1LL);

  bool is_leader;
  std::cout << "Is this device the leader? (1 = yes, 0 = no): ";
  std::cin >> is_leader;

  Drone drone(radio, is_leader);
  drone.printDroneInfo();

  if (is_leader) {
    DroneIdType current_target = 1;

    while (true) {
      PermissionToSendPacket perm;
      perm.type = PacketType::PERMISSION_TO_SEND;
      perm.target_drone_id = current_target;
      perm.timestamp = static_cast<uint32_t>(std::time(nullptr));

      bool sent = radio.send(&perm, sizeof(perm));
      std::cout << "[LEADER] Granted permission to drone "
                << static_cast<int>(current_target) << " -> "
                << (sent ? "OK" : "FAIL") << "\n";

      TelemetryPacket telemetry;
      if (radio.receive(&telemetry, sizeof(telemetry))) {
        std::cout << "[LEADER] Telemetry received from "
                  << static_cast<int>(telemetry.drone_id)
                  << " | Battery: " << telemetry.battery_voltage
                  << "V Altitude: " << telemetry.altitude << "\n";
      } else {
        std::cout << "[LEADER] No telemetry received.\n";
      }

      current_target++;
      if (current_target > MAX_DRONES)
        current_target = 1;

      sleep(1);
    }
  } else {
    int heartbeat_counter = 0;

    while (true) {
      PermissionToSendPacket perm;
      if (radio.receive(&perm, sizeof(perm)) &&
          perm.type == PacketType::PERMISSION_TO_SEND &&
          perm.target_drone_id == drone.getTempId()) {

        drone.updateSensors(100, 100, 100, 5, 5, 5, 50.0f, 3.8f);
        drone.sendTelemetry();
      }

      if (++heartbeat_counter % 10 == 0) {
        HeartbeatPacket hb;
        hb.type = PacketType::HEARTBEAT;
        hb.source_drone_id = drone.getTempId();
        hb.timestamp = static_cast<uint32_t>(std::time(nullptr));

        radio.send(&hb, sizeof(hb));
        std::cout << "[DRONE] Heartbeat sent.\n";
      }

      usleep(200000); // 200ms
    }
  }

  return 0;
}
