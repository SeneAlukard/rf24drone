#include <chrono>  // Zaman damgaları ve beklemeler için
#include <fstream> // Added for file logging
#include <iomanip> // std::hex, std::setfill, std::setw için (adres yazdırmada)
#include <iostream>
#include <optional> // std::optional için
#include <string>
#include <thread> // std::this_thread::sleep_for için
#include <vector>

#include "drone.hpp" // Kendi Drone sınıfımız
#include "radio.hpp" // Radyo haberleşme fonksiyonlarımız
// packets.hpp, radio.hpp tarafından zaten dahil ediliyor, ancak doğrudan
// kullanıyorsak açıkça eklemek iyi bir pratiktir.
#include "packets.hpp" // Paket tanımlamalarımız

// --- Yardımcı Fonksiyonlar ---

// Mevcut zaman damgasını milisaniye cinsinden almak için basit bir fonksiyon
uint32_t getCurrentTimestamp() {
  return static_cast<uint32_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}

// Bir RF adresini okunabilir formatta yazdırmak için yardımcı fonksiyon
// MODIFIED to take an ostream
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
  out << std::dec; // Onluk sayı sistemine geri dön
}

// --- Örnek RF Adresleri ---
// Not: Bu adresler normalde radio.cpp içinde tanımlanıp radio.hpp'de extern
// olarak bildirilmeli veya bir konfigürasyon sistemi ile yönetilmelidir. Bu
// main.cpp örneği için burada tanımlıyoruz.
const uint8_t THIS_DRONE_LISTEN_ADDR_P0[RF24_ADDRESS_WIDTH] = {
    0xE8, 0xE8, 0xF0, 0xF0, 0xE1}; // Bu drone'un P0'da dinleyeceği adres
const uint8_t THIS_DRONE_LISTEN_ADDR_P1[RF24_ADDRESS_WIDTH] = {
    0xC1, 0xC2, 0xC3, 0xC4,
    0xC5}; // Bu drone'un P1'de dinleyeceği adres (örn: broadcast)
const uint8_t TARGET_DEVICE_ADDR[RF24_ADDRESS_WIDTH] = {
    0xA1, 0xB2, 0xC3, 0xD4,
    0xE5}; // Veri gönderilecek örnek hedef adres (örn: Yer İstasyonu)

int main() {
  std::ofstream log_file("drone_log.txt",
                         std::ios::app); // Open log file in append mode

  if (!log_file.is_open()) {
    std::cerr << "FATAL ERROR: Could not open log file 'drone_log.txt'!"
              << std::endl;
    std::cout << "FATAL ERROR: Could not open log file 'drone_log.txt'!"
              << std::endl;
    return 1; // Exit if log file cannot be opened
  }

  uint32_t ts_start = getCurrentTimestamp();
  log_file << "[" << ts_start
           << "] ===== Drone NRF24 Programi Baslatiliyor =====" << std::endl;
  std::cout << "===== Drone NRF24 Programi Baslatiliyor =====" << std::endl;

  // 1. Bu cihaz için bir Drone nesnesi oluştur
  Drone this_drone(0, false,
                   "Drone-001"); // initial_rssi, is_leader_init, initial_name

  // Simülasyon: Bu drone'a bir ağ ID'si atandığını varsayalım (gerçekte
  // JoinResponse ile gelir)
  DroneIdType drone_network_id = 1; // packets.hpp'den DroneIdType
  this_drone.setNetworkId(drone_network_id);

  // Log drone info to file
  log_file << "[" << getCurrentTimestamp() << "] --- Drone Bilgileri (Log) ---"
           << std::endl;
  log_file << "[" << getCurrentTimestamp()
           << "]   İçsel ID: " << this_drone.getId() << std::endl;
  log_file << "[" << getCurrentTimestamp() << "]   Ağ ID: ";
  if (this_drone.getNetworkId()) {
    log_file << static_cast<int>(*(this_drone.getNetworkId()));
  } else {
    log_file << "Atanmamış";
  }
  log_file << std::endl;
  log_file << "[" << getCurrentTimestamp()
           << "]   İsim: " << this_drone.getName() << std::endl;
  log_file << "[" << getCurrentTimestamp()
           << "]   RSSI: " << static_cast<int>(this_drone.getRssi())
           << std::endl;
  log_file << "[" << getCurrentTimestamp() << "]   Lider Durumu: "
           << (this_drone.isLeader() ? "Lider" : "Lider Değil") << std::endl;
  log_file << "[" << getCurrentTimestamp() << "] ----------------------------"
           << std::endl;

  this_drone.printDroneInfo(); // Keep console output for drone info
  std::cout << std::endl;

  // 2. RF Haberleşmesini Başlat
  uint8_t rf_channel = 108; // Kullanılacak RF kanalı (0-125)

  log_file << "[" << getCurrentTimestamp()
           << "] RF Haberlesmesi baslatiliyor..." << std::endl;
  log_file << "[" << getCurrentTimestamp()
           << "]   Kanal: " << static_cast<int>(rf_channel) << std::endl;
  std::cout << "RF Haberlesmesi baslatiliyor..." << std::endl;
  std::cout << "  Kanal: " << static_cast<int>(rf_channel) << std::endl;

  // Log and print listening addresses
  printRfAddress(log_file,
                 "  Dinleme Adresi P0 (Log): ", THIS_DRONE_LISTEN_ADDR_P0);
  log_file << std::endl;
  printRfAddress(std::cout, "  Dinleme Adresi P0: ", THIS_DRONE_LISTEN_ADDR_P0);
  std::cout << std::endl;

  printRfAddress(log_file,
                 "  Dinleme Adresi P1 (Log): ", THIS_DRONE_LISTEN_ADDR_P1);
  log_file << std::endl;
  printRfAddress(std::cout, "  Dinleme Adresi P1: ", THIS_DRONE_LISTEN_ADDR_P1);
  std::cout << std::endl;

  printRfAddress(log_file,
                 "  Varsayilan Hedef Adres (Log): ", TARGET_DEVICE_ADDR);
  log_file << std::endl;
  printRfAddress(std::cout, "  Varsayilan Hedef Adres: ", TARGET_DEVICE_ADDR);
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
  printRadioDetails(); // radio.cpp'deki fonksiyonu çağır - output to console
                       // via printf
  log_file << "[" << getCurrentTimestamp() << "] printRadioDetails() cagrildi."
           << std::endl;
  std::cout << std::endl;

  // 3. Ana Döngü
  log_file << "[" << getCurrentTimestamp() << "] Ana dongu baslatiliyor..."
           << std::endl;
  std::cout << "Ana dongu baslatiliyor... (Ctrl+C ile cikabilirsiniz)"
            << std::endl;
  auto last_heartbeat_send_time = std::chrono::steady_clock::now();
  const auto heartbeat_interval =
      std::chrono::seconds(5); // Her 5 saniyede bir heartbeat

  while (true) {
    // === A. Gelen Paketleri Kontrol Et ve İşle ===
    uint8_t pipe_num_available;
    if (isDataAvailable(&pipe_num_available)) {
      uint8_t received_data_buffer[32]; // NRF24 maksimum payload boyutu
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
        // std::cerr << "Veri okuma hatasi (readData false döndürdü)." <<
        // std::endl; // Optional console
      }
    }

    // === B. Periyodik Olarak Heartbeat Paketi Gönder ===
    auto current_time = std::chrono::steady_clock::now();
    if (current_time - last_heartbeat_send_time >= heartbeat_interval) {
      last_heartbeat_send_time = current_time;

      if (this_drone.getNetworkId()) {
        HeartbeatPacket hb_pkt;
        hb_pkt.source_drone_id = this_drone.getNetworkId().value();
        hb_pkt.timestamp = getCurrentTimestamp();

        uint32_t hb_ts =
            hb_pkt.timestamp; // Use packet's timestamp for logging consistency

        log_file << "[" << hb_ts << "] Heartbeat gonderiliyor -> ";
        printRfAddress(log_file, "", TARGET_DEVICE_ADDR); // Log address
        log_file << " ... ";

        std::cout << "[" << hb_ts << "] Heartbeat gonderiliyor -> ";
        printRfAddress(std::cout, "", TARGET_DEVICE_ADDR); // Console address
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

    // === C. Diğer İşlemler ===
    // (No changes here for logging unless specific actions are added)

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  uint32_t ts_end = getCurrentTimestamp();
  log_file << "[" << ts_end << "] Program sonlandiriliyor." << std::endl;
  std::cout << "Program sonlandiriliyor." << std::endl;

  log_file.close();
  return 0;
}
