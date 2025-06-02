#pragma once

#include <cstddef>
#include <cstdint> // Standart integer tipleri için (uint8_t, uint16_t, vb.)

// --- ID Tipi Tanımı ---
using DroneIdType = uint8_t; // Drone ID'si için 8-bit unsigned integer

// --- Paket Tipleri Enum ---
enum class PacketType : uint8_t {
  UNDEFINED = 0,
  JOIN_REQUEST = 1,
  JOIN_RESPONSE = 2,
  COMMAND = 3,
  TELEMETRY = 4,
  HEARTBEAT = 5,
  LEADER_ANNOUNCEMENT = 6,
  LEADER_REQUEST = 7,
};

// --- Sabitler ---
constexpr size_t MAX_COMMAND_LENGTH = 20;
constexpr size_t MAX_NODE_NAME_LENGTH =
    16; // İstenen isim için maksimum uzunluk

// --- Paket Yapıları ---

struct CommandPacket {
  PacketType type = PacketType::COMMAND; // Paket tipini varsayılan olarak ata
  DroneIdType target_drone_id;
  uint32_t timestamp;
  char command[MAX_COMMAND_LENGTH];
};

struct TelemetryPacket {
  PacketType type = PacketType::TELEMETRY; // Paket tipini varsayılan olarak ata
  DroneIdType source_drone_id;
  uint32_t timestamp;
  float altitude;
  float battery_voltage;
  // int8_t rssi_to_leader_or_server;
};

struct JoinRequestPacket {
  PacketType type =
      PacketType::JOIN_REQUEST; // Paket tipini varsayılan olarak ata
  uint32_t timestamp;
  DroneIdType temp_id; // Geçici bir ID veya MAC adresinin bir kısmı olabilir
  char requested_name[MAX_NODE_NAME_LENGTH]; // Drone'un ağda kullanmak istediği
                                             // isim
};

struct JoinResponsePacket {
  PacketType type =
      PacketType::JOIN_RESPONSE; // Paket tipini varsayılan olarak ata
  DroneIdType assigned_id;
  DroneIdType current_leader_id;
  uint32_t timestamp;
  // Ağ ayarları (kullanılacak RF kanalı, şifreleme anahtarı parçası vb.)
};

struct HeartbeatPacket {
  PacketType type = PacketType::HEARTBEAT; // Paket tipini varsayılan olarak ata
  DroneIdType source_drone_id;
  uint32_t timestamp;
  // bool is_leader;
};

struct LeaderAnnouncementPacket {
  PacketType type =
      PacketType::LEADER_ANNOUNCEMENT; // Paket tipini varsayılan olarak ata
  DroneIdType new_leader_id;
  uint32_t timestamp;
};

struct LeaderRequestPacket {
  PacketType type =
      PacketType::LEADER_REQUEST; // Paket tipini varsayılan olarak ata
  DroneIdType requesting_drone_id;
  uint32_t timestamp;
  // DroneIdType last_known_leader_id;
  // int8_t reason_code;
};

/*
### Dikkat Edilmesi Gerekenler (packets.hpp için):

1.  **Paket Tipi Tanımlama Yöntemi**:
    * Her struct'ın içine `PacketType type = PacketType::[ilgili_tip];` şeklinde
bir üye ekledik. Bu, gelen bir paketin türünü belirlemenin bir yoludur.
    * Alternatif olarak, tüm bu paketleri bir `union` içinde toplayıp, başına
`PacketType` içeren genel bir yapı (`GenericPacket` gibi) koymak da bir
yöntemdir. Bu, özellikle bellek optimizasyonu veya farklı paket türleri arasında
doğrudan C-stili cast işlemleri yapılacaksa tercih edilebilir.

2.  **AES Şifreleme**:
    * Bu yapılar şifrelenmemiş (plaintext) veriyi temsil eder.
    * Bu yapılar önce bir byte dizisine (serialization), sonra bu byte dizisi
şifrelenmelidir.
    * Alıcı tarafında ise önce şifreli veri çözülmeli, sonra byte dizisinden
tekrar bu struct yapılarına dönüştürülmelidir (deserialization).
    * IV (Initialization Vector) ve MAC (Message Authentication Code) gibi ek
verilerin nasıl iletileceği düşünülmelidir.

3.  **Serialization/Deserialization**:
    * Struct'ları byte dizisine ve byte dizisini struct'lara çevirmek için
fonksiyonlara ihtiyacınız olacak. Özellikle `float`, `uint32_t` gibi çok byte'lı
üyeler için byte sırasına (endianness) dikkat edin.

4.  **Maksimum Veri Boyutu (Payload Size)**:
    * NRF24L01+ modülünün maksimum veri boyutu genellikle 32 byte'dır.
    * Paket yapılarının (şifreleme, IV, MAC gibi ek bilgilerle birlikte) bu
sınırı aşmamasına dikkat edin. Aşarsa, fragmentation ve reassembly mekanizmaları
gerekebilir. Özellikle `char requested_name[MAX_NODE_NAME_LENGTH]` gibi alanlar
paket boyutunu etkileyebilir.
*/
