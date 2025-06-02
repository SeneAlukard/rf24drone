#include <chrono>  // Zaman damgaları ve beklemeler için
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
void printRfAddress(const char *label, const uint8_t *address) {
  std::cout << label;
  if (address) {
    for (size_t i = 0; i < RF24_ADDRESS_WIDTH; ++i) {
      std::cout << std::hex << std::setfill('0') << std::setw(2)
                << static_cast<int>(address[i]);
      if (i < RF24_ADDRESS_WIDTH - 1)
        std::cout << ":";
    }
  } else {
    std::cout << "NULL";
  }
  std::cout << std::dec; // Onluk sayı sistemine geri dön
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
  std::cout << "===== Drone NRF24 Programi Baslatiliyor =====" << std::endl;

  // 1. Bu cihaz için bir Drone nesnesi oluştur
  Drone this_drone(0, false,
                   "Drone-001"); // initial_rssi, is_leader_init, initial_name

  // Simülasyon: Bu drone'a bir ağ ID'si atandığını varsayalım (gerçekte
  // JoinResponse ile gelir)
  DroneIdType drone_network_id = 1; // packets.hpp'den DroneIdType
  this_drone.setNetworkId(drone_network_id);
  this_drone.printDroneInfo();
  std::cout << std::endl;

  // 2. RF Haberleşmesini Başlat
  uint8_t rf_channel = 108; // Kullanılacak RF kanalı (0-125)
  std::cout << "RF Haberlesmesi baslatiliyor..." << std::endl;
  std::cout << "  Kanal: " << static_cast<int>(rf_channel) << std::endl;
  printRfAddress("  Dinleme Adresi P0: ", THIS_DRONE_LISTEN_ADDR_P0);
  std::cout << std::endl;
  printRfAddress("  Dinleme Adresi P1: ", THIS_DRONE_LISTEN_ADDR_P1);
  std::cout << std::endl;
  printRfAddress("  Varsayilan Hedef Adres: ", TARGET_DEVICE_ADDR);
  std::cout << std::endl;

  if (!setupRFCommunication(rf_channel, THIS_DRONE_LISTEN_ADDR_P0,
                            THIS_DRONE_LISTEN_ADDR_P1)) {
    std::cerr
        << "ANA HATA: RF Haberlesmesi baslatilamadi. Program sonlandiriliyor."
        << std::endl;
    return 1;
  }
  std::cout << "RF Haberlesmesi basariyla baslatildi." << std::endl;
  printRadioDetails(); // radio.cpp'deki fonksiyonu çağır
  std::cout << std::endl;

  // 3. Ana Döngü
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
        std::cout << "[" << getCurrentTimestamp() << "] Pipe "
                  << static_cast<int>(pipe_num_available) << " uzerinden "
                  << static_cast<int>(received_len) << " byte veri alindi."
                  << std::endl;

        if (received_len > 0 && received_len <= sizeof(received_data_buffer)) {
          // Gelen verinin ilk byte'ını PacketType olarak yorumla
          // Bu, packets.hpp'deki her struct'ın ilk üyesinin 'PacketType type;'
          // olmasına dayanır.
          PacketType type =
              *(reinterpret_cast<PacketType *>(received_data_buffer));

          std::cout << "  Paket Tipi: ";
          switch (type) {
          case PacketType::COMMAND: {
            std::cout << "COMMAND" << std::endl;
            if (received_len >= sizeof(CommandPacket)) {
              const CommandPacket *pkt =
                  reinterpret_cast<const CommandPacket *>(received_data_buffer);
              std::cout << "    Hedef ID: "
                        << static_cast<int>(pkt->target_drone_id)
                        << ", Zaman: " << pkt->timestamp << ", Komut: '"
                        << pkt->command << "'" << std::endl;
            } else {
              std::cout
                  << "    Uyarı: Paket boyutu CommandPacket için yetersiz!"
                  << std::endl;
            }
            break;
          }
          case PacketType::TELEMETRY: {
            std::cout << "TELEMETRY" << std::endl;
            if (received_len >= sizeof(TelemetryPacket)) {
              const TelemetryPacket *pkt =
                  reinterpret_cast<const TelemetryPacket *>(
                      received_data_buffer);
              std::cout << "    Kaynak ID: "
                        << static_cast<int>(pkt->source_drone_id)
                        << ", Zaman: " << pkt->timestamp
                        << ", İrtifa: " << pkt->altitude
                        << ", Batarya: " << pkt->battery_voltage << std::endl;
            } else {
              std::cout
                  << "    Uyarı: Paket boyutu TelemetryPacket için yetersiz!"
                  << std::endl;
            }
            break;
          }
          case PacketType::HEARTBEAT: {
            std::cout << "HEARTBEAT" << std::endl;
            if (received_len >= sizeof(HeartbeatPacket)) {
              const HeartbeatPacket *pkt =
                  reinterpret_cast<const HeartbeatPacket *>(
                      received_data_buffer);
              std::cout << "    Kaynak ID: "
                        << static_cast<int>(pkt->source_drone_id)
                        << ", Zaman: " << pkt->timestamp << std::endl;
            } else {
              std::cout
                  << "    Uyarı: Paket boyutu HeartbeatPacket için yetersiz!"
                  << std::endl;
            }
            break;
          }
          // Diğer paket türleri için case'ler buraya eklenebilir:
          // JOIN_REQUEST, JOIN_RESPONSE, LEADER_ANNOUNCEMENT, LEADER_REQUEST
          default:
            std::cout << "BILINMEYEN veya ISLENMEYEN ("
                      << static_cast<int>(type) << ")" << std::endl;
            // Ham veriyi hex olarak yazdırmak isteyebilirsiniz (debug için)
            std::cout << "    Ham Veri (max 8 byte): ";
            for (uint8_t i = 0; i < std::min((uint8_t)8, received_len); ++i) {
              std::cout << std::hex << std::setfill('0') << std::setw(2)
                        << (int)received_data_buffer[i] << " ";
            }
            std::cout << std::dec << std::endl;
            break;
          }
        }
      } else {
        // readData false döndürdüyse (genellikle veri yokken çağrılmaz ama bir
        // sorun olabilir) std::cerr << "Veri okuma hatasi (readData false
        // döndürdü)." << std::endl;
      }
    }

    // === B. Periyodik Olarak Heartbeat Paketi Gönder ===
    auto current_time = std::chrono::steady_clock::now();
    if (current_time - last_heartbeat_send_time >= heartbeat_interval) {
      last_heartbeat_send_time = current_time;

      if (this_drone.getNetworkId()) { // Sadece ağ ID'si varsa heartbeat gönder
        HeartbeatPacket hb_pkt;
        // hb_pkt.type zaten struct içinde varsayılan olarak
        // PacketType::HEARTBEAT atanmıştı.
        hb_pkt.source_drone_id = this_drone.getNetworkId().value();
        hb_pkt.timestamp = getCurrentTimestamp();

        std::cout << "[" << getCurrentTimestamp()
                  << "] Heartbeat gonderiliyor -> ";
        printRfAddress("", TARGET_DEVICE_ADDR);
        std::cout << " ... ";

        if (sendHeartbeatPacket(
                hb_pkt, TARGET_DEVICE_ADDR)) { // radio.hpp'deki fonksiyon
          std::cout << "Basarili." << std::endl;
        } else {
          std::cout << "HATA!" << std::endl;
        }
      }
    }

    // === C. Diğer İşlemler ===
    // Bu bölümde kullanıcıdan komut alma, sensör okuma gibi işlemler
    // yapılabilir. Örnek: Basit bir klavye girdisiyle komut gönderme (çok
    // basit)
    /*
    if (konsoldan_girdi_var_mi()) {
        std::string user_command_str = konsoldan_oku();
        CommandPacket user_cmd_pkt;
        user_cmd_pkt.target_drone_id = HEDEF_DRONE_ID; // Uygun bir hedef ID
        user_cmd_pkt.timestamp = getCurrentTimestamp();
        strncpy(user_cmd_pkt.command, user_command_str.c_str(),
    MAX_COMMAND_LENGTH -1); user_cmd_pkt.command[MAX_COMMAND_LENGTH -1] = '\0';
        sendCommandPacket(user_cmd_pkt, TARGET_DEVICE_ADDR);
    }
    */

    // CPU'yu gereksiz yere meşgul etmemek için kısa bir bekleme
    std::this_thread::sleep_for(std::chrono::milliseconds(20)); // 20ms bekleme
  }

  std::cout << "Program sonlandiriliyor." << std::endl;
  return 0;
}
