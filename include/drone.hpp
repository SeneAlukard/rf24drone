#pragma once

#include "packets.hpp"
#include "radio.hpp"
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>

class Drone {
public:
  Drone(RadioInterface &radio_ref, bool is_leader_init = false,
        const std::string &initial_name = "UnknownDrone");

  DroneIdType getTempId() const;
  std::optional<DroneIdType> getNetworkId() const;
  bool isLeader() const;
  const std::string &getName() const;

  void setNetworkId(DroneIdType net_id);
  void clearNetworkId();
  void setLeaderStatus(bool status);
  void setName(const std::string &new_name);

  void updateSensors(int16_t ax, int16_t ay, int16_t az, int16_t gx, int16_t gy,
                     int16_t gz, float altitude, float battery_voltage);

  void handleIncoming(); // Gelen paketlere göre tepki verir
  void sendTelemetry();  // Sadece izin aldıysa gönderir

  void printDroneInfo() const;

private:
  RadioInterface &radio;
  DroneIdType temp_id_;
  std::optional<DroneIdType> network_id_;
  bool is_leader_;
  bool has_permission_to_send_ = false;
  std::string name_;
  TelemetryPacket telemetry;

  void handleCommand(const CommandPacket &cmd);
};
