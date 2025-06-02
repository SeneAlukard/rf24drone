#include "../include/drone.hpp"
#include <iostream> // std::cout ve std::endl kullanabilmek için

// Drone sınıfının statik üye değişkeninin tanımlanması ve başlangıç değeri
// atanması İçsel ID'ler 0'dan başlayacak şekilde ayarlayalım.
uint16_t Drone::next_id_counter_ = 0;

// --- Yapıcı Metot (Constructor) ---
Drone::Drone(int8_t initial_rssi, bool is_leader_init,
             const std::string &initial_name)
    : id_(next_id_counter_++),   // Benzersiz içsel ID ata ve sayacı artır
      network_id_(std::nullopt), // network_id_ başlangıçta atanmamış (boş)
      rssi_(initial_rssi), is_leader_(is_leader_init), name_(initial_name) {
  // Yapıcı metot gövdesinde ek işlemler yapılabilir, şimdilik boş.
  // std::cout << "Drone nesnesi oluşturuldu. İçsel ID: " << id_ << ", İsim: "
  // << name_ << std::endl;
}

// --- Getter Metotları ---
uint16_t Drone::getId() const { return id_; }

std::optional<DroneIdType> Drone::getNetworkId() const { return network_id_; }

int8_t Drone::getRssi() const { return rssi_; }

bool Drone::isLeader() const { return is_leader_; }

const std::string &Drone::getName() const { return name_; }

// --- Setter Metotları ---
void Drone::setNetworkId(DroneIdType net_id) { network_id_ = net_id; }

void Drone::clearNetworkId() { network_id_ = std::nullopt; }

void Drone::setRssi(int8_t new_rssi) { rssi_ = new_rssi; }

void Drone::setLeaderStatus(bool status) { is_leader_ = status; }

void Drone::setName(const std::string &new_name) { name_ = new_name; }

// --- Bilgi Yazdırma Metotları ---
void Drone::printId() const { std::cout << "İçsel ID: " << id_; }

void Drone::printNetworkId() const {
  std::cout << "Ağ ID: ";
  if (network_id_) {
    // DroneIdType uint8_t olduğu için doğrudan int'e cast ederek yazdıralım ki
    // karakter olarak yorumlanmasın
    std::cout << static_cast<int>(*network_id_);
  } else {
    std::cout << "Atanmamış";
  }
}

void Drone::printName() const { std::cout << "İsim: " << name_; }

void Drone::printRssi() const {
  // int8_t de karakter olarak yorumlanabilir, int'e cast edelim
  std::cout << "RSSI: " << static_cast<int>(rssi_);
}

void Drone::printLeaderStatus() const {
  std::cout << "Lider Durumu: " << (is_leader_ ? "Lider" : "Lider Değil");
}

void Drone::printDroneInfo() const {
  std::cout << "--- Drone Bilgileri ---" << std::endl;
  printId();
  std::cout << std::endl;
  printNetworkId();
  std::cout << std::endl;
  printName();
  std::cout << std::endl;
  printRssi();
  std::cout << std::endl;
  printLeaderStatus();
  std::cout << std::endl;
  std::cout << "-----------------------" << std::endl;
}
