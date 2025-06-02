#include <chrono>   // For timestamps and delays
#include <fstream>  // For file logging
#include <iomanip>  // For std::hex, std::setfill, std::setw (in printRfAddress)
#include <iostream> // For console output
#include <optional> // For std::optional (used in Drone class)
#include <string>   // For std::string
#include <thread>   // For std::this_thread::sleep_for
#include <vector>   // Not directly used in this version, but often useful

#include "drone.hpp"   // Drone class definition
#include "packets.hpp" // Packet structure definitions
#include "radio.hpp" // Radio communication functions (setupRFCommunication, sendXPacket, etc.)

// --- Drone Identity Configuration ---
// IMPORTANT: Define which drone this instance is.
//            Uncomment ONLY ONE of the following lines for each drone.
//            Then, recompile and upload to that specific drone.

// #define IS_DRONE_1
// #define IS_DRONE_2

// --- Sanity Check for Drone Identity ---
#if !defined(IS_DRONE_1) && !defined(IS_DRONE_2)
#error                                                                         \
    "CRITICAL ERROR: Drone identity not defined! Please uncomment either IS_DRONE_1 or IS_DRONE_2 at the top of main.cpp and recompile."
#elif defined(IS_DRONE_1) && defined(IS_DRONE_2)
#error                                                                         \
    "CRITICAL ERROR: Both IS_DRONE_1 and IS_DRONE_2 are defined! Please uncomment only ONE and recompile."
#endif
// ----------------------------------

// --- RF Address Definitions for Two Drones ---
// These addresses define how the drones talk to each other.
// Drone 1 listens on DRONE1_LISTEN_ADDRESS and sends to DRONE2_LISTEN_ADDRESS.
// Drone 2 listens on DRONE2_LISTEN_ADDRESS and sends to DRONE1_LISTEN_ADDRESS.
const uint8_t DRONE1_LISTEN_ADDRESS[RF24_ADDRESS_WIDTH] = {0xE1, 0xE1, 0xE1,
                                                           0xE1, 0xE1};
const uint8_t DRONE2_LISTEN_ADDRESS[RF24_ADDRESS_WIDTH] = {0xD2, 0xD2, 0xD2,
                                                           0xD2, 0xD2};
// --------------------------------------------

// --- Conditionally Set Addresses, Drone Name, and Network ID ---
#if defined(IS_DRONE_1)
const uint8_t *const THIS_DRONE_LISTEN_ADDR_P0 = DRONE1_LISTEN_ADDRESS;
const uint8_t *const THIS_DRONE_LISTEN_ADDR_P1 =
    nullptr; // Pipe 1 not used in this basic setup
const uint8_t *const TARGET_DEVICE_ADDR = DRONE2_LISTEN_ADDRESS;
std::string THIS_DRONE_NAME = "Drone-Alpha";
DroneIdType THIS_DRONE_NETWORK_ID = 1;
#elif defined(IS_DRONE_2)
const uint8_t *const THIS_DRONE_LISTEN_ADDR_P0 = DRONE2_LISTEN_ADDRESS;
const uint8_t *const THIS_DRONE_LISTEN_ADDR_P1 =
    nullptr; // Pipe 1 not used in this basic setup
const uint8_t *const TARGET_DEVICE_ADDR = DRONE1_LISTEN_ADDRESS;
std::string THIS_DRONE_NAME = "Drone-Beta";
DroneIdType THIS_DRONE_NETWORK_ID = 2;
#endif
// -------------------------------------------------

// Global log file stream
std::ofstream log_file;

// --- Helper Functions ---
uint32_t getCurrentTimestamp() {
  return static_cast<uint32_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}

// Modified to write to any ostream (e.g., cout or log_file)
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
  out << std::dec; // Switch back to decimal for subsequent output
}

// Function to log messages to both console and file
void logMessage(const std::string &message, bool is_error = false) {
  uint32_t ts = getCurrentTimestamp();
  std::ostream &console_stream = is_error ? std::cerr : std::cout;

  console_stream << "[" << ts << "] " << message << std::endl;
  if (log_file.is_open()) {
    log_file << "[" << ts << "] " << message << std::endl;
  }
}

int main() {
  // Open log file in append mode
  log_file.open("drone_log.txt", std::ios::app);
  if (!log_file.is_open()) {
    // If log file can't be opened, we can only use cerr/cout
    std::cerr << "[" << getCurrentTimestamp()
              << "] FATAL ERROR: Could not open log file 'drone_log.txt'!"
              << std::endl;
    // We might choose to exit or continue without file logging. For now,
    // continue.
  }

  logMessage("===== Drone NRF24 Program (" + THIS_DRONE_NAME +
             ") Baslatiliyor =====");

  // 1. Create Drone object for this device
  Drone this_drone(0, false, THIS_DRONE_NAME);
  this_drone.setNetworkId(THIS_DRONE_NETWORK_ID);

  // Log and print drone info
  std::string drone_info_msg = "--- Drone Bilgileri ---\n";
  drone_info_msg += "  Isim: " + this_drone.getName() + "\n";
  drone_info_msg +=
      "  Ag ID: " +
      (this_drone.getNetworkId()
           ? std::to_string(static_cast<int>(*this_drone.getNetworkId()))
           : "Atanmamis") +
      "\n";
  drone_info_msg += "  Icsel ID: " + std::to_string(this_drone.getId()) + "\n";
  drone_info_msg +=
      "  RSSI: " + std::to_string(static_cast<int>(this_drone.getRssi())) +
      "\n";
  drone_info_msg +=
      "  Lider Durumu: " +
      std::string(this_drone.isLeader() ? "Lider" : "Lider Degil");
  logMessage(drone_info_msg);
  // this_drone.printDroneInfo(); // This prints directly to cout, logMessage
  // handles combined logging

  // 2. Initialize RF Communication
  uint8_t rf_channel =
      108; // RF Channel (0-125). MUST BE THE SAME ON ALL DRONES.

  std::string rf_setup_msg = "RF Haberlesmesi baslatiliyor...\n";
  rf_setup_msg +=
      "  Kanal: " + std::to_string(static_cast<int>(rf_channel)) + "\n";

  std::stringstream p0_addr_ss, target_addr_ss;
  printRfAddress(p0_addr_ss, "", THIS_DRONE_LISTEN_ADDR_P0);
  rf_setup_msg += "  Dinleme Adresi P0: " + p0_addr_ss.str() + "\n";

  printRfAddress(target_addr_ss, "", TARGET_DEVICE_ADDR);
  rf_setup_msg += "  Hedef Adres: " + target_addr_ss.str();
  logMessage(rf_setup_msg);

  if (!setupRFCommunication(rf_channel, THIS_DRONE_LISTEN_ADDR_P0,
                            THIS_DRONE_LISTEN_ADDR_P1)) {
    logMessage(
        "ANA HATA: RF Haberlesmesi baslatilamadi. Program sonlandiriliyor.",
        true);
    if (log_file.is_open())
      log_file.close();
    return 1; // Exit if RF setup fails
  }
  logMessage("RF Haberlesmesi basariyla baslatildi.");

  logMessage("printRadioDetails() cagriliyor (konsola yazacak)...");
  printRadioDetails(); // This function from radio.cpp prints directly to
                       // console (using printf)
  logMessage("printRadioDetails() cagrildi.");

  // 3. Main Loop
  logMessage("Ana dongu baslatiliyor... (Ctrl+C ile cikabilirsiniz)");
  auto last_heartbeat_send_time = std::chrono::steady_clock::now();
  const auto heartbeat_interval =
      std::chrono::seconds(5); // Send heartbeat every 5 seconds

  while (true) {
    // === A. Check for and Process Incoming Packets ===
    uint8_t pipe_num_available;
    if (isDataAvailable(&pipe_num_available)) {
      uint8_t received_data_buffer[32]; // NRF24 max payload size
      uint8_t received_len = 0;

      if (readData(received_data_buffer, sizeof(received_data_buffer),
                   received_len)) {
        std::string received_msg_hdr =
            "Pipe " + std::to_string(static_cast<int>(pipe_num_available)) +
            " uzerinden " + std::to_string(static_cast<int>(received_len)) +
            " byte veri alindi.";
        logMessage(received_msg_hdr);

        if (received_len > 0 && received_len <= sizeof(received_data_buffer)) {
          PacketType type =
              *(reinterpret_cast<PacketType *>(received_data_buffer));
          std::string packet_details_msg = "  Paket Tipi: ";

          switch (type) {
          case PacketType::COMMAND: {
            packet_details_msg += "COMMAND\n";
            if (received_len >= sizeof(CommandPacket)) {
              const CommandPacket *pkt =
                  reinterpret_cast<const CommandPacket *>(received_data_buffer);
              packet_details_msg +=
                  "    Hedef ID: " +
                  std::to_string(static_cast<int>(pkt->target_drone_id)) +
                  ", Zaman: " + std::to_string(pkt->timestamp) + ", Komut: '" +
                  pkt->command + "'";
            } else {
              packet_details_msg +=
                  "    Uyarı: Paket boyutu CommandPacket için yetersiz!";
            }
            break;
          }
          case PacketType::TELEMETRY: {
            packet_details_msg += "TELEMETRY\n";
            if (received_len >= sizeof(TelemetryPacket)) {
              const TelemetryPacket *pkt =
                  reinterpret_cast<const TelemetryPacket *>(
                      received_data_buffer);
              packet_details_msg +=
                  "    Kaynak ID: " +
                  std::to_string(static_cast<int>(pkt->source_drone_id)) +
                  ", Zaman: " + std::to_string(pkt->timestamp) +
                  ", İrtifa: " + std::to_string(pkt->altitude) +
                  ", Batarya: " + std::to_string(pkt->battery_voltage);
            } else {
              packet_details_msg +=
                  "    Uyarı: Paket boyutu TelemetryPacket için yetersiz!";
            }
            break;
          }
          case PacketType::HEARTBEAT: {
            packet_details_msg += "HEARTBEAT\n";
            if (received_len >= sizeof(HeartbeatPacket)) {
              const HeartbeatPacket *pkt =
                  reinterpret_cast<const HeartbeatPacket *>(
                      received_data_buffer);
              packet_details_msg +=
                  "    Kaynak ID: " +
                  std::to_string(static_cast<int>(pkt->source_drone_id)) +
                  ", Zaman: " + std::to_string(pkt->timestamp);
            } else {
              packet_details_msg +=
                  "    Uyarı: Paket boyutu HeartbeatPacket için yetersiz!";
            }
            break;
          }
          // Add cases for other packet types (JOIN_REQUEST, etc.) if needed
          default:
            packet_details_msg += "BILINMEYEN veya ISLENMEYEN (" +
                                  std::to_string(static_cast<int>(type)) +
                                  ")\n";
            packet_details_msg += "    Ham Veri (max 8 byte): ";
            std::stringstream hex_data_ss;
            for (uint8_t i = 0; i < std::min((uint8_t)8, received_len); ++i) {
              hex_data_ss << std::hex << std::setfill('0') << std::setw(2)
                          << (int)received_data_buffer[i] << " ";
            }
            packet_details_msg += hex_data_ss.str();
            break;
          }
          logMessage(packet_details_msg);
        }
      } else {
        logMessage("Veri okuma hatasi (readData false döndürdü).", true);
      }
    }

    // === B. Periodically Send Heartbeat Packet ===
    auto current_time = std::chrono::steady_clock::now();
    if (current_time - last_heartbeat_send_time >= heartbeat_interval) {
      last_heartbeat_send_time = current_time;

      if (this_drone.getNetworkId()) {
        HeartbeatPacket hb_pkt;
        // hb_pkt.type is set by default in struct definition
        hb_pkt.source_drone_id = this_drone.getNetworkId().value();
        hb_pkt.timestamp = getCurrentTimestamp();

        std::stringstream target_addr_ss_hb;
        printRfAddress(target_addr_ss_hb, "", TARGET_DEVICE_ADDR);
        std::string hb_send_msg =
            "Heartbeat (" + this_drone.getName() +
            " ID:" + std::to_string(static_cast<int>(hb_pkt.source_drone_id)) +
            ") gonderiliyor -> " + target_addr_ss_hb.str() + " ... ";

        if (sendHeartbeatPacket(hb_pkt, TARGET_DEVICE_ADDR)) {
          hb_send_msg += "Basarili.";
        } else {
          hb_send_msg += "HATA!";
        }
        logMessage(
            hb_send_msg,
            !sendHeartbeatPacket(
                hb_pkt, TARGET_DEVICE_ADDR)); // Log as error if send failed
      }
    }

    // === C. Other Operations (e.g., sensor reading, command input) ===
    // Placeholder for future logic

    // Short delay to prevent busy-waiting and reduce CPU load
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  logMessage("Program (" + THIS_DRONE_NAME + ") sonlandiriliyor.");
  if (log_file.is_open()) {
    log_file.close();
  }
  return 0;
}
