#include <chrono>  // Zaman damgaları ve beklemeler için
#include <fstream> // For file logging
#include <iomanip> // std::hex, std::setfill, std::setw için (adres yazdırmada)
#include <iostream>
#include <optional> // std::optional için
#include <string>
#include <thread> // std::this_thread::sleep_for için
#include <vector>

#include "drone.hpp"   // Kendi Drone sınıfımız
#include "packets.hpp" // Paket tanımlamalarımız
#include "radio.hpp"   // Radyo haberleşme fonksiyonlarımız

// --- Drone Identity Configuration ---
// Define which drone this instance is. Uncomment ONE of the following lines.
// For the first drone, uncomment #define IS_DRONE_1
// For the second drone, uncomment #define IS_DRONE_2
// Then compile and run on the respective drone.
#define IS_DRONE_1
// #define IS_DRONE_2
// ----------------------------------

// --- RF Address Definitions for Two Drones ---
// Ensure these are unique for your pair of drones.
const uint8_t DRONE1_LISTEN_ADDRESS[RF24_ADDRESS_WIDTH] = {0xE1, 0xE1, 0xE1,
                                                           0xE1, 0xE1};
const uint8_t DRONE2_LISTEN_ADDRESS[RF24_ADDRESS_WIDTH] = {0xD2, 0xD2, 0xD2,
                                                           0xD2, 0xD2};
// --------------------------------------------

// --- Conditionally Set Addresses and Drone Name ---
#if defined(IS_DRONE_1)
const uint8_t *const THIS_DRONE_LISTEN_ADDR_P0 = DRONE1_LISTEN_ADDRESS;
const uint8_t *const THIS_DRONE_LISTEN_ADDR_P1 =
    nullptr; // Not using P1 for this example
const uint8_t *const TARGET_DEVICE_ADDR =
    DRONE2_LISTEN_ADDRESS; // Drone 1 sends to Drone 2
std::string THIS_DRONE_NAME = "Drone-Alpha";
#elif defined(IS_DRONE_2)
const uint8_t *const THIS_DRONE_LISTEN_ADDR_P0 = DRONE2_LISTEN_ADDRESS;
const uint8_t *const THIS_DRONE_LISTEN_ADDR_P1 =
    nullptr; // Not using P1 for this example
const uint8_t *const TARGET_DEVICE_ADDR =
    DRONE1_LISTEN_ADDRESS; // Drone 2 sends to Drone 1
std::string THIS_DRONE_NAME = "Drone-Beta";
#else
#error                                                                         \
    "Drone identity not defined! Please define IS_DRONE_1 or IS_DRONE_2 at the top of main.cpp."
// Default fallback to prevent compilation errors, but communication will likely
// fail.
const uint8_t *const THIS_DRONE_LISTEN_ADDR_P0 = DRONE1_LISTEN_ADDRESS;
const uint8_t *const THIS_DRONE_LISTEN_ADDR_P1 = nullptr;
const uint8_t *const TARGET_DEVICE_ADDR = DRONE2_LISTEN_ADDRESS;
std::string THIS_DRONE_NAME = "Drone-Default";
#endif
// -------------------------------------------------

// --- Yardımcı Fonksiyonlar ---
uint32_t getCurrentTimestamp() {
  return static_cast<uint32_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}

void printRfAddress(std::ostream &out, const char *label,
                    const uint8_t *address) {
  out << label;
  if (address) {
    for (size_t i = 0; i < RF24_ADDRESS_WIDTH; ++i) {
      out << std::hex << std::setfill('0') << std::setw(2)
          << static_cast<int>(address[i]);
      if (i < RF24_ADDRESS_WIDTH - 1)
        out << ":";
    }
  } else {
    out << "NULL";
  }
  out << std::dec;
}

int main() {
  std::ofstream log_file("drone_log.txt", std::ios::app);

  if (!log_file.is_open()) {
    std::cerr << "FATAL ERROR: Could not open log file 'drone_log.txt'!"
              << std::endl;
    std::cout << "FATAL ERROR: Could not open log file 'drone_log.txt'!"
              << std::endl;
    return 1;
  }

  uint32_t ts_start = getCurrentTimestamp();
  log_file << "[" << ts_start << "] ===== Drone NRF24 Program ("
           << THIS_DRONE_NAME << ") Baslatiliyor =====" << std::endl;
  std::cout << "===== Drone NRF24 Program (" << THIS_DRONE_NAME
            << ") Baslatiliyor =====" << std::endl;

  Drone this_drone(0, false, THIS_DRONE_NAME);

  DroneIdType drone_network_id = 0; // Default
#if defined(IS_DRONE_1)
  drone_network_id = 1;
#elif defined(IS_DRONE_2)
  drone_network_id = 2;
#endif
  this_drone.setNetworkId(drone_network_id);

  log_file << "[" << getCurrentTimestamp() << "] --- Drone Bilgileri (Log) ---"
           << std::endl;
  log_file << "[" << getCurrentTimestamp()
           << "]   İsim: " << this_drone.getName() << std::endl;
  log_file << "[" << getCurrentTimestamp() << "]   Ağ ID: ";
  if (this_drone.getNetworkId()) {
    log_file << static_cast<int>(*(this_drone.getNetworkId()));
  } else {
    log_file << "Atanmamış";
  }
  log_file << std::endl;
  // Add other drone info to log if needed (ID, RSSI, Leader status)
  log_file << "[" << getCurrentTimestamp() << "] ----------------------------"
           << std::endl;

  this_drone.printDroneInfo();
  std::cout << std::endl;

  uint8_t rf_channel = 108;

  log_file << "[" << getCurrentTimestamp()
           << "] RF Haberlesmesi baslatiliyor..." << std::endl;
  log_file << "[" << getCurrentTimestamp()
           << "]   Kanal: " << static_cast<int>(rf_channel) << std::endl;
  std::cout << "RF Haberlesmesi baslatiliyor..." << std::endl;
  std::cout << "  Kanal: " << static_cast<int>(rf_channel) << std::endl;

  printRfAddress(log_file,
                 "  Dinleme Adresi P0 (Log): ", THIS_DRONE_LISTEN_ADDR_P0);
  log_file << std::endl;
  printRfAddress(std::cout, "  Dinleme Adresi P0: ", THIS_DRONE_LISTEN_ADDR_P0);
  std::cout << std::endl;

  if (THIS_DRONE_LISTEN_ADDR_P1 != nullptr) {
    printRfAddress(log_file,
                   "  Dinleme Adresi P1 (Log): ", THIS_DRONE_LISTEN_ADDR_P1);
    log_file << std::endl;
    printRfAddress(std::cout,
                   "  Dinleme Adresi P1: ", THIS_DRONE_LISTEN_ADDR_P1);
    std::cout << std::endl;
  }

  printRfAddress(log_file, "  Hedef Adres (Log): ", TARGET_DEVICE_ADDR);
  log_file << std::endl;
  printRfAddress(std::cout, "  Hedef Adres: ", TARGET_DEVICE_ADDR);
  std::cout << std::endl;

  if (!setupRFCommunication(rf_channel, THIS_DRONE_LISTEN_ADDR_P0,
                            THIS_DRONE_LISTEN_ADDR_P1)) {
    uint32_t err_ts = getCurrentTimestamp();
    log_file
        << "[" << err_ts
        << "] ANA HATA: RF Haberlesmesi baslatilamadi. Program sonlandiriliyor."
        << std::endl;
    std::cerr
        << "ANA HATA: RF Haberlesmesi baslatilamadi. Program sonlandiriliyor."
        << std::endl;
    log_file.close();
    return 1;
  }
  log_file << "[" << getCurrentTimestamp()
           << "] RF Haberlesmesi basariyla baslatildi." << std::endl;
  std::cout << "RF Haberlesmesi basariyla baslatildi." << std::endl;

  log_file << "[" << getCurrentTimestamp()
           << "] printRadioDetails() cagriliyor (konsola yazacak)..."
           << std::endl;
  printRadioDetails();
  log_file << "[" << getCurrentTimestamp() << "] printRadioDetails() cagrildi."
           << std::endl;
  std::cout << std::endl;

  log_file << "[" << getCurrentTimestamp() << "] Ana dongu baslatiliyor..."
           << std::endl;
  std::cout << "Ana dongu baslatiliyor... (Ctrl+C ile cikabilirsiniz)"
            << std::endl;
  auto last_heartbeat_send_time = std::chrono::steady_clock::now();
  const auto heartbeat_interval = std::chrono::seconds(5);

  while (true) {
    uint8_t pipe_num_available;
    if (isDataAvailable(&pipe_num_available)) {
      uint8_t received_data_buffer[32];
      uint8_t received_len = 0;

      if (readData(received_data_buffer, sizeof(received_data_buffer),
                   received_len)) {
        uint32_t recv_ts = getCurrentTimestamp();
        log_file << "[" << recv_ts << "] Pipe "
                 << static_cast<int>(pipe_num_available) << " uzerinden "
                 << static_cast<int>(received_len) << " byte veri alindi."
                 << std::endl;
        std::cout << "[" << recv_ts << "] Pipe "
                  << static_cast<int>(pipe_num_available) << " uzerinden "
                  << static_cast<int>(received_len) << " byte veri alindi."
                  << std::endl;

        if (received_len > 0 && received_len <= sizeof(received_data_buffer)) {
          PacketType type =
              *(reinterpret_cast<PacketType *>(received_data_buffer));

          log_file << "  Paket Tipi: ";
          std::cout << "  Paket Tipi: ";
          switch (type) {
          case PacketType::COMMAND: {
            log_file << "COMMAND" << std::endl;
            std::cout << "COMMAND" << std::endl;
            if (received_len >= sizeof(CommandPacket)) {
              const CommandPacket *pkt =
                  reinterpret_cast<const CommandPacket *>(received_data_buffer);
              log_file << "    Hedef ID: "
                       << static_cast<int>(pkt->target_drone_id)
                       << ", Zaman: " << pkt->timestamp << ", Komut: '"
                       << pkt->command << "'" << std::endl;
              std::cout << "    Hedef ID: "
                        << static_cast<int>(pkt->target_drone_id)
                        << ", Zaman: " << pkt->timestamp << ", Komut: '"
                        << pkt->command << "'" << std::endl;
            } else {
              log_file << "    Uyarı: Paket boyutu CommandPacket için yetersiz!"
                       << std::endl;
              std::cout
                  << "    Uyarı: Paket boyutu CommandPacket için yetersiz!"
                  << std::endl;
            }
            break;
          }
          case PacketType::TELEMETRY: {
            log_file << "TELEMETRY" << std::endl;
            std::cout << "TELEMETRY" << std::endl;
            if (received_len >= sizeof(TelemetryPacket)) {
              const TelemetryPacket *pkt =
                  reinterpret_cast<const TelemetryPacket *>(
                      received_data_buffer);
              log_file << "    Kaynak ID: "
                       << static_cast<int>(pkt->source_drone_id)
                       << ", Zaman: " << pkt->timestamp
                       << ", İrtifa: " << pkt->altitude
                       << ", Batarya: " << pkt->battery_voltage << std::endl;
              std::cout << "    Kaynak ID: "
                        << static_cast<int>(pkt->source_drone_id)
                        << ", Zaman: " << pkt->timestamp
                        << ", İrtifa: " << pkt->altitude
                        << ", Batarya: " << pkt->battery_voltage << std::endl;
            } else {
              log_file
                  << "    Uyarı: Paket boyutu TelemetryPacket için yetersiz!"
                  << std::endl;
              std::cout
                  << "    Uyarı: Paket boyutu TelemetryPacket için yetersiz!"
                  << std::endl;
            }
            break;
          }
          case PacketType::HEARTBEAT: {
            log_file << "HEARTBEAT" << std::endl;
            std::cout << "HEARTBEAT" << std::endl;
            if (received_len >= sizeof(HeartbeatPacket)) {
              const HeartbeatPacket *pkt =
                  reinterpret_cast<const HeartbeatPacket *>(
                      received_data_buffer);
              log_file << "    Kaynak ID: "
                       << static_cast<int>(pkt->source_drone_id)
                       << ", Zaman: " << pkt->timestamp << std::endl;
              std::cout << "    Kaynak ID: "
                        << static_cast<int>(pkt->source_drone_id)
                        << ", Zaman: " << pkt->timestamp << std::endl;
            } else {
              log_file
                  << "    Uyarı: Paket boyutu HeartbeatPacket için yetersiz!"
                  << std::endl;
              std::cout
                  << "    Uyarı: Paket boyutu HeartbeatPacket için yetersiz!"
                  << std::endl;
            }
            break;
          }
          // ... (other packet types: JOIN_REQUEST, JOIN_RESPONSE,
          // LEADER_ANNOUNCEMENT, LEADER_REQUEST) Add cases for these if they
          // are expected and need specific logging. E.g.:
          /*
          case PacketType::JOIN_REQUEST:
            log_file << "JOIN_REQUEST" << std::endl;
            std::cout << "JOIN_REQUEST" << std::endl;
            // Add specific logging for JoinRequestPacket fields
            break;
          */
          default:
            log_file << "BILINMEYEN veya ISLENMEYEN (" << static_cast<int>(type)
                     << ")" << std::endl;
            std::cout << "BILINMEYEN veya ISLENMEYEN ("
                      << static_cast<int>(type) << ")" << std::endl;
            log_file << "    Ham Veri (max 8 byte): ";
            std::cout << "    Ham Veri (max 8 byte): ";
            for (uint8_t i = 0; i < std::min((uint8_t)8, received_len); ++i) {
              log_file << std::hex << std::setfill('0') << std::setw(2)
                       << (int)received_data_buffer[i] << " ";
              std::cout << std::hex << std::setfill('0') << std::setw(2)
                        << (int)received_data_buffer[i] << " ";
            }
            log_file << std::dec << std::endl;
            std::cout << std::dec << std::endl;
            break;
          }
        }
      } else {
        uint32_t read_err_ts = getCurrentTimestamp();
        log_file << "[" << read_err_ts
                 << "] Veri okuma hatasi (readData false döndürdü)."
                 << std::endl;
      }
    }

    auto current_time = std::chrono::steady_clock::now();
    if (current_time - last_heartbeat_send_time >= heartbeat_interval) {
      last_heartbeat_send_time = current_time;

      if (this_drone.getNetworkId()) {
        HeartbeatPacket hb_pkt;
        hb_pkt.source_drone_id = this_drone.getNetworkId().value();
        hb_pkt.timestamp = getCurrentTimestamp();

        uint32_t hb_ts = hb_pkt.timestamp;

        log_file << "[" << hb_ts << "] Heartbeat (" << this_drone.getName()
                 << " ID:" << static_cast<int>(hb_pkt.source_drone_id)
                 << ") gonderiliyor -> ";
        printRfAddress(log_file, "", TARGET_DEVICE_ADDR);
        log_file << " ... ";

        std::cout << "[" << hb_ts << "] Heartbeat (" << this_drone.getName()
                  << " ID:" << static_cast<int>(hb_pkt.source_drone_id)
                  << ") gonderiliyor -> ";
        printRfAddress(std::cout, "", TARGET_DEVICE_ADDR);
        std::cout << " ... ";

        if (sendHeartbeatPacket(hb_pkt, TARGET_DEVICE_ADDR)) {
          log_file << "Basarili." << std::endl;
          std::cout << "Basarili." << std::endl;
        } else {
          log_file << "HATA!" << std::endl;
          std::cout << "HATA!" << std::endl;
        }
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  uint32_t ts_end = getCurrentTimestamp();
  log_file << "[" << ts_end << "] Program (" << THIS_DRONE_NAME
           << ") sonlandiriliyor." << std::endl;
  std::cout << "Program (" << THIS_DRONE_NAME << ") sonlandiriliyor."
            << std::endl;

  log_file.close();
  return 0;
}
