#include "../include/drone.hpp"
#include "../include/packets.hpp"
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <queue>
#include <array>

Drone::Drone(RadioInterface &radio_ref, bool is_leader_init,
             const std::string &initial_name)
    : radio(radio_ref), is_leader_(is_leader_init), name_(initial_name) {
  std::srand(static_cast<unsigned int>(std::time(nullptr)));
  temp_id_ = static_cast<DroneIdType>(std::rand() % 200 + 1); // 1–200 arası
  telemetry = TelemetryPacket{}; // güvenli sıfırlama
  rx_queue_ = {};
}

DroneIdType Drone::getTempId() const { return temp_id_; }

std::optional<DroneIdType> Drone::getNetworkId() const { return network_id_; }

bool Drone::isLeader() const { return is_leader_; }

const std::string &Drone::getName() const { return name_; }

std::optional<DroneIdType> Drone::getCurrentLeaderId() const {
  return current_leader_id_;
}

void Drone::setCurrentLeaderId(std::optional<DroneIdType> id) {
  current_leader_id_ = id;
}

bool Drone::hasRoleChanged() const { return role_changed_; }

void Drone::clearRoleChanged() { role_changed_ = false; }

void Drone::setNetworkId(DroneIdType net_id) { network_id_ = net_id; }

void Drone::clearNetworkId() { network_id_ = std::nullopt; }

void Drone::setLeaderStatus(bool status) { is_leader_ = status; }

void Drone::setName(const std::string &new_name) { name_ = new_name; }

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
  telemetry.battery_voltage = battery_voltage;
  telemetry.altitude = altitude;
}

size_t Drone::packetSize(PacketType type) const {
  switch (type) {
  case PacketType::COMMAND:
    return sizeof(CommandPacket);
  case PacketType::TELEMETRY:
    return sizeof(TelemetryPacket);
  case PacketType::JOIN_REQUEST:
    return sizeof(JoinRequestPacket);
  case PacketType::JOIN_RESPONSE:
    return sizeof(JoinResponsePacket);
  case PacketType::HEARTBEAT:
    return sizeof(HeartbeatPacket);
  case PacketType::LEADER_ANNOUNCEMENT:
    return sizeof(LeaderAnnouncementPacket);
  case PacketType::PERMISSION_TO_SEND:
    return sizeof(PermissionToSendPacket);
  case PacketType::LEADER_REQUEST:
    return sizeof(LeaderRequestPacket);
  default:
    return sizeof(PacketType);
  }
}

void Drone::pollRadio() {
  PacketType peek;
  while (radio.receive(&peek, sizeof(PacketType), true)) {
    RawPacket pkt{};
    pkt.type = peek;
    pkt.size = packetSize(peek);
    radio.receive(pkt.data.data(), pkt.size);
    telemetry.rpd = radio.testRPD() ? 1 : 0;
    rx_queue_.push(pkt);
  }
}

void Drone::sendTelemetry() {
  if (!has_permission_to_send_)
    return;

  total_sends_++;
  bool success = radio.send(&telemetry, sizeof(telemetry));
  if (!success)
    failed_sends_++;
  telemetry.retries = radio.getARC();
  telemetry.link_quality =
      100.0f * (1.0f - static_cast<float>(failed_sends_) /
                             static_cast<float>(total_sends_));
  has_permission_to_send_ = false; // izni kullandı
}

void Drone::handleIncoming() {
  pollRadio();

  while (!rx_queue_.empty()) {
    RawPacket pkt = rx_queue_.front();
    rx_queue_.pop();

    switch (pkt.type) {
    case PacketType::COMMAND: {
      if (pkt.size == sizeof(CommandPacket)) {
        CommandPacket cmd{};
        std::memcpy(&cmd, pkt.data.data(), sizeof(cmd));
        handleCommand(cmd);
      }
      break;
    }
    case PacketType::PERMISSION_TO_SEND: {
      if (pkt.size == sizeof(PermissionToSendPacket)) {
        PermissionToSendPacket perm{};
        std::memcpy(&perm, pkt.data.data(), sizeof(perm));
        if (perm.target_drone_id == network_id_.value_or(temp_id_))
          has_permission_to_send_ = true;
      }
      break;
    }
    case PacketType::LEADER_ANNOUNCEMENT: {
      if (pkt.size == sizeof(LeaderAnnouncementPacket)) {
        LeaderAnnouncementPacket ann{};
        std::memcpy(&ann, pkt.data.data(), sizeof(ann));
        handleLeaderAnnouncement(ann);
      }
      break;
    }
    case PacketType::JOIN_RESPONSE: {
      if (pkt.size == sizeof(JoinResponsePacket)) {
        JoinResponsePacket resp{};
        std::memcpy(&resp, pkt.data.data(), sizeof(resp));
        handleJoinResponse(resp);
      }
      break;
    }
    case PacketType::TELEMETRY: {
      if (pkt.size == sizeof(TelemetryPacket)) {
        TelemetryPacket tlm{};
        std::memcpy(&tlm, pkt.data.data(), sizeof(tlm));
        handleTelemetry(tlm);
      }
      break;
    }
    case PacketType::HEARTBEAT: {
      if (pkt.size == sizeof(HeartbeatPacket)) {
        HeartbeatPacket hb{};
        std::memcpy(&hb, pkt.data.data(), sizeof(hb));
        handleHeartbeat(hb);
      }
      break;
    }
    case PacketType::LEADER_REQUEST: {
      if (pkt.size == sizeof(LeaderRequestPacket)) {
        LeaderRequestPacket req{};
        std::memcpy(&req, pkt.data.data(), sizeof(req));
        handleLeaderRequest(req);
      }
      break;
    }
    case PacketType::UNDEFINED: {
      handleUndefined();
      break;
    }
    default:
      break;
    }
  }
}

void Drone::handleCommand(const CommandPacket &cmd) {
  DroneIdType self_id = network_id_.value_or(temp_id_);
  uint32_t now = static_cast<uint32_t>(std::time(nullptr));

  if (cmd.target_drone_id != self_id)
    return;

  if (now - cmd.timestamp > 3)
    return; // gecikmesi 3 saniyeden büyükse yoksay

  std::cout << "[Komut] " << cmd.command << std::endl;
}

void Drone::handleLeaderAnnouncement(const LeaderAnnouncementPacket &ann) {
  DroneIdType self_id = network_id_.value_or(temp_id_);
  bool was_leader = is_leader_;

  if (ann.new_leader_id == self_id) {
    is_leader_ = true;
    current_leader_id_ = self_id;
  } else {
    is_leader_ = false;
    current_leader_id_ = ann.new_leader_id;
  }

  if (was_leader != is_leader_)
    role_changed_ = true;
}

void Drone::printDroneInfo() const {
  std::cout << "=== Drone Bilgileri ===\n";
  std::cout << "Temp ID: " << static_cast<int>(temp_id_) << "\n";
  std::cout << "Net ID : ";
  if (network_id_)
    std::cout << static_cast<int>(*network_id_) << "\n";
  else
    std::cout << "(atanmamış)\n";
  std::cout << "İsim   : " << name_ << "\n";
  std::cout << "Lider? : " << (is_leader_ ? "Evet" : "Hayır") << "\n";
}

void Drone::handleJoinResponse(const JoinResponsePacket &resp) {
  std::cout << "[JoinResponse] ID " << static_cast<int>(resp.assigned_id)
            << " Channel " << static_cast<int>(resp.assigned_channel)
            << " Leader " << static_cast<int>(resp.current_leader_id)
            << std::endl;
}

void Drone::handleTelemetry(const TelemetryPacket &tlm) {
  std::cout << "[Telemetry] Drone " << static_cast<int>(tlm.drone_id)
            << " Altitude " << tlm.altitude << std::endl;
}

void Drone::handleHeartbeat(const HeartbeatPacket &hb) {
  std::cout << "[Heartbeat] from " << static_cast<int>(hb.source_drone_id)
            << std::endl;
}

void Drone::handleLeaderRequest(const LeaderRequestPacket &req) {
  std::cout << "[LeaderRequest] from " << static_cast<int>(req.drone_id)
            << std::endl;
}

void Drone::handleUndefined() { std::cout << "UNDEFINED MESSAGE COME" << std::endl; }
