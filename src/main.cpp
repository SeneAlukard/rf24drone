#include "packets.hpp"
#include "radio.hpp"
#include <cstdio>
#include <iostream>
#include <unistd.h>

#define CE_PIN 22
#define CSN_PIN 0

int main() {
  RadioInterface radio(CE_PIN, CSN_PIN); // CE=22, CSN=0

  if (!radio.begin()) {
    std::cerr << "Radio başlatılamadı!" << std::endl;
    return 1;
  }

  radio.configure(85, RadioDataRate::MEDIUM_RATE);
  radio.setAddress(0xF0F0F0F0D2LL, 0xF0F0F0F0E1LL);

  // Kullanıcıdan liderlik durumu al
  bool is_leader = false;
  std::cout << "Bu cihaz lider mi? (1 = evet, 0 = hayır): ";
  std::cin >> is_leader;

  if (is_leader) {
    std::cout << "Lider modunda başlatılıyor..." << std::endl;

    PermissionToSendPacket packet;
    packet.type = PacketType::PERMISSION_TO_SEND;
    packet.target_drone_id = 1;
    packet.timestamp = 123456;

    while (true) {
      bool sent = radio.send(&packet, sizeof(packet));
      std::cout << "İzin paketi gönderildi: " << (sent ? "EVET" : "HAYIR")
                << std::endl;

      sleep(1);
    }
  } else {
    std::cout << "Drone dinleme modunda..." << std::endl;

    CommandPacket incoming;
    while (true) {
      if (radio.receive(&incoming, sizeof(incoming))) {
        std::cout << "Komut alındı → hedef ID: "
                  << static_cast<int>(incoming.target_drone_id)
                  << ", Komut: " << incoming.command << std::endl;
      }

      usleep(100000); // 100ms
    }
  }

  return 0;
}
