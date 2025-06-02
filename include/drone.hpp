#pragma once

#include "packets.hpp" // DroneIdType tanımını alabilmek için
#include <cstdint>
#include <iostream> // print metodları için std::cout kullanılacaksa
#include <optional> // std::optional kullanabilmek için
#include <string>

class Drone {
private:
  uint16_t
      id_; // Drone'un benzersiz içsel sistem ID'si (yapıcıda otomatik artar)
  std::optional<DroneIdType> network_id_; // Drone'un NRF24 ağındaki ID'si
                                          // (packets.hpp'den DroneIdType)
  int8_t rssi_;                           // Sinyal gücü
  bool is_leader_;                        // Bu drone lider mi?
  std::string name_;                      // Drone'un ismi

  static uint16_t
      next_id_counter_; // Bir sonraki atanacak içsel ID için statik sayaç

public:
  // Yapıcı Metot: initial_socket parametresi kaldırıldı.
  // network_id_ başlangıçta atanmamış (std::nullopt) olacak veya sonradan set
  // edilecek.
  Drone(int8_t initial_rssi = 0, bool is_leader_init = false,
        const std::string &initial_name = "UnknownDrone");

  // Getter Metotları
  uint16_t getId() const;                          // İçsel sistem ID'si
  std::optional<DroneIdType> getNetworkId() const; // NRF24 Ağ ID'si
  int8_t getRssi() const;
  bool isLeader() const;
  const std::string &getName() const;

  // Setter Metotları
  void setNetworkId(DroneIdType net_id); // Ağ ID'sini atar
  void clearNetworkId(); // Ağ ID'sini "atanmamış" olarak işaretler
  void setRssi(int8_t new_rssi);
  void setLeaderStatus(bool status);
  void setName(const std::string &new_name);

  // Bilgi Yazdırma Metotları
  void printId() const;
  void printNetworkId() const; // network_id_ için yazdırma metodu eklendi
  void printName() const;
  void printRssi() const;
  void printLeaderStatus() const;
  void
  printDroneInfo() const; // Tüm bilgileri yazdırır (printSocket yerine
                          // printNetworkId'yi çağıracak şekilde güncellenmeli)
};
