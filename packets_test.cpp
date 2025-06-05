#include "radio.hpp"
#include "packets.hpp"
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>
#include <array>

// Pin definitions for a single device that has separate TX and RX modules
// Only the Drone_A pins are used in this test. They should match your wiring.
#define DRONE_A_TX_CE 27
#define DRONE_A_TX_CSN 10
#define DRONE_A_RX_CE 22
#define DRONE_A_RX_CSN 0

static constexpr uint64_t ADDR_A_TX = 0xF0F0F0F0AAULL;

static size_t packetSize(PacketType t) {
  switch (t) {
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

static void handlePacket(PacketType t, const std::array<uint8_t, 32>& buf) {
  switch (t) {
  case PacketType::COMMAND: {
    CommandPacket pkt{};
    std::memcpy(&pkt, buf.data(), sizeof(pkt));
    std::cout << "CMD -> " << pkt.command << "\n";
    break;
  }
  case PacketType::TELEMETRY: {
    TelemetryPacket pkt{};
    std::memcpy(&pkt, buf.data(), sizeof(pkt));
    std::cout << "TLM -> id " << static_cast<int>(pkt.drone_id) << " alt "
              << pkt.altitude << "\n";
    break;
  }
  case PacketType::JOIN_REQUEST: {
    JoinRequestPacket pkt{};
    std::memcpy(&pkt, buf.data(), sizeof(pkt));
    std::cout << "JOIN_REQ -> temp " << static_cast<int>(pkt.temp_id) << "\n";
    break;
  }
  case PacketType::JOIN_RESPONSE: {
    JoinResponsePacket pkt{};
    std::memcpy(&pkt, buf.data(), sizeof(pkt));
    std::cout << "JOIN_RESP -> assigned " << static_cast<int>(pkt.assigned_id)
              << "\n";
    break;
  }
  case PacketType::HEARTBEAT: {
    HeartbeatPacket pkt{};
    std::memcpy(&pkt, buf.data(), sizeof(pkt));
    std::cout << "HEARTBEAT -> src " << static_cast<int>(pkt.source_drone_id)
              << "\n";
    break;
  }
  case PacketType::LEADER_ANNOUNCEMENT: {
    LeaderAnnouncementPacket pkt{};
    std::memcpy(&pkt, buf.data(), sizeof(pkt));
    std::cout << "LEADER_ANNOUNCE -> new " << static_cast<int>(pkt.new_leader_id)
              << "\n";
    break;
  }
  case PacketType::PERMISSION_TO_SEND: {
    PermissionToSendPacket pkt{};
    std::memcpy(&pkt, buf.data(), sizeof(pkt));
    std::cout << "PERMISSION -> target " << static_cast<int>(pkt.target_drone_id)
              << "\n";
    break;
  }
  case PacketType::LEADER_REQUEST: {
    LeaderRequestPacket pkt{};
    std::memcpy(&pkt, buf.data(), sizeof(pkt));
    std::cout << "LEADER_REQ -> id " << static_cast<int>(pkt.drone_id) << "\n";
    break;
  }
  case PacketType::UNDEFINED:
    std::cout << "UNDEFINED" << std::endl;
    break;
  }
}

static void sender(RadioInterface& radio) {
  CommandPacket cmd{};
  cmd.target_drone_id = 1;
  cmd.timestamp = 1;
  std::strcpy(cmd.command, "test");

  TelemetryPacket tlm{};
  tlm.drone_id = 2;
  tlm.timestamp = 2;
  tlm.altitude = 100.0f;

  JoinRequestPacket jr{};
  jr.timestamp = 3;
  jr.temp_id = 3;
  std::strcpy(jr.requested_name, "node");

  JoinResponsePacket jresp{};
  jresp.assigned_id = 4;
  jresp.current_leader_id = 1;
  jresp.assigned_channel = 90;
  jresp.timestamp = 4;

  HeartbeatPacket hb{};
  hb.source_drone_id = 5;
  hb.timestamp = 5;

  LeaderAnnouncementPacket ann{};
  ann.new_leader_id = 2;
  ann.timestamp = 6;

  PermissionToSendPacket perm{};
  perm.target_drone_id = 6;
  perm.timestamp = 7;

  LeaderRequestPacket lreq{};
  lreq.drone_id = 7;
  lreq.timestamp = 8;

  std::vector<std::pair<const void*, size_t>> pkts{
      {&cmd, sizeof(cmd)},   {&tlm, sizeof(tlm)},       {&jr, sizeof(jr)},
      {&jresp, sizeof(jresp)}, {&hb, sizeof(hb)},         {&ann, sizeof(ann)},
      {&perm, sizeof(perm)}, {&lreq, sizeof(lreq)}};

  for (auto& p : pkts) {
    radio.send(p.first, p.second);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  PacketType undef = PacketType::UNDEFINED;
  radio.send(&undef, sizeof(undef));
}

static void receiver(RadioInterface& radio, size_t expected) {
  size_t count = 0;
  while (count < expected) {
    PacketType peek;
    if (!radio.receive(&peek, sizeof(peek), true)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      continue;
    }
    std::array<uint8_t, 32> buf{};
    radio.receive(buf.data(), packetSize(peek));
    handlePacket(peek, buf);
    ++count;
  }
}

int main() {
  RadioInterface radio(DRONE_A_TX_CE, DRONE_A_TX_CSN, DRONE_A_RX_CE, DRONE_A_RX_CSN);

  if (!radio.begin()) {
    std::cerr << "Failed to initialize radio" << std::endl;
    return 1;
  }

  radio.configure(90, RadioDataRate::MEDIUM_RATE);
  // Use the same address for TX and RX so the device can send to itself.
  radio.setAddress(ADDR_A_TX, ADDR_A_TX);

  std::thread t(receiver, std::ref(radio), 9);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  sender(radio);
  t.join();
  return 0;
}

