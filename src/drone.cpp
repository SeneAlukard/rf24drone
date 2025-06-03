#include "../include/drone.hpp"
#include <ctime>

Drone::Drone(RadioInterface &radio_ref, bool is_leader_init,
             const std::string &initial_name)
    : radio(radio_ref), is_leader_(is_leader_init), name_(initial_name) {
  std::srand(static_cast<unsigned int>(std::time(nullptr)));
  temp_id_ = static_cast<DroneIdType>(std::rand() % 254 + 1);
  network_id_ = std::nullopt;
}

DroneIdType Drone::getTempId() const { return temp_id_; }

void Drone::printDroneInfo() const {
  std::cout << "[DRONE INFO] Temp ID: " << static_cast<int>(temp_id_)
            << ", Name: " << name_ << (is_leader_ ? " [LEADER]\n" : "\n");
}

void Drone::updateSensors(int16_t ax, int16_t ay, int16_t az, int16_t gx,
                          int16_t gy, int16_t gz, float altitude,
                          float battery_voltage) {
  telemetry.type = PacketType::TELEMETRY;
  telemetry.drone_id = network_id_.value_or(temp_id_);
  telemetry.timestamp = static_cast<uint32_t>(std::time(nullptr));
  telemetry.acceleration_x = ax;
  telemetry.acceleration_y = ay;
  telemetry.acceleration_z = az;
  telemetry.gyroscope_x = gx;
  telemetry.gyroscope_y = gy;
  telemetry.gyroscope_z = gz;
  telemetry.altitude = altitude;
  telemetry.battery_voltage = battery_voltage;
}

void Drone::sendTelemetry() { radio.send(&telemetry, sizeof(telemetry)); }
