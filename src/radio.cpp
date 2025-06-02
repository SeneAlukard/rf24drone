#include "../include/radio.hpp" // Kendi başlık dosyamız (RF24.h ve packets.hpp'yi de içerir)
#include <cstring>              // memcpy gibi hafıza işlemleri için (gerekirse)
#include <iostream> // printRadioDetails ve genel debug için

// --- Global RF24 Nesnesinin Tanımı ---
// radio.hpp'de 'extern RF24 radio;' olarak bildirilmişti.
// Kullanıcı tarafından belirtilen pinler: CE=22, CSN=7
RF24 radio(22, 0);

// --- Örnek RF Adreslerinin Tanımları (radio.hpp'de extern olarak
// bildirilmişti) --- Bu adresleri kendi ağ yapılandırmanıza göre
// belirlemelisiniz. const uint8_t GROUND_STATION_ADDRESS[RF24_ADDRESS_WIDTH] =
// {0xF0, 0xF0, 0xF0, 0xF0, 0xE1}; const uint8_t
// DRONE_BROADCAST_ADDRESS[RF24_ADDRESS_WIDTH] = {0xF0, 0xF0, 0xF0, 0xF0, 0xD2};
// Diğer adres tanımları...

// --- Fonksiyon Gövdeleri ---

bool setupRFCommunication(uint8_t rf_channel,
                          const uint8_t *my_listen_address_p0,
                          const uint8_t *my_listen_address_p1) {
  if (!radio.begin()) {
    std::cerr << "HATA: Radyo başlatılamadı! Bağlantıları kontrol edin."
              << std::endl;
    return false;
  }

  radio.setChannel(rf_channel); // RF Kanalını ayarla (0-125)

  // Veri Hızı: Uzun menzil için RF24_250KBPS, daha yüksek hız için RF24_1MBPS
  // veya RF24_2MBPS.
  radio.setDataRate(RF24_1MBPS); // Varsayılan olarak 1MBPS kullanalım

  // Güç Seviyesi: RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setPALevel(RF24_PA_MAX); // Maksimum güçte yayın

  radio.setAutoAck(true);        // Otomatik Onay (ACK) paketlerini etkinleştir
  radio.enableDynamicPayloads(); // Dinamik paket boyutlarını etkinleştir (32
                                 // byte'a kadar)
  radio.setRetries(
      5, 15); // Tekrar deneme gecikmesi (5*250µs = 1250µs) ve sayısı (15)

  // Dinleme pipe'larını aç
  // Pipe 0: Genellikle ana dinleme adresi
  if (my_listen_address_p0) {
    radio.openReadingPipe(0, my_listen_address_p0);
  }
  // Pipe 1: İkincil dinleme adresi (örn: broadcast veya özel bir grup için)
  if (my_listen_address_p1) {
    radio.openReadingPipe(1, my_listen_address_p1);
  }
  // Diğer pipe'lar (2-5) da benzer şekilde açılabilir:
  // radio.openReadingPipe(2, address_pipe2);

  radio.startListening(); // Dinlemeye başla

  std::cout << "Radyo başlatıldı. Kanal: " << (int)rf_channel
            << ", Dinleme Adresi P0: [adres] " // Adresleri yazdırmak için bir
                                               // helper fonksiyon gerekebilir
            << std::endl;
  // printRadioDetails(); // Başlangıç ayarlarını yazdırmak iyi bir fikir
  // olabilir

  return true; // Başarılı kurulum
}

bool openListeningPipe(uint8_t pipe_number, const uint8_t *address) {
  if (pipe_number > 5) { // Pipe 0 özel, diğerleri 1-5
    std::cerr << "HATA: Geçersiz pipe numarası: " << (int)pipe_number
              << std::endl;
    return false;
  }
  if (!address) {
    std::cerr << "HATA: openListeningPipe için geçersiz adres." << std::endl;
    return false;
  }
  radio.openReadingPipe(pipe_number, address);
  // RF24 kütüphanesi openReadingPipe için doğrudan bir başarı durumu döndürmez,
  // ancak adresin geçerli olup olmadığını kontrol edebiliriz.
  // Genellikle bu işlem başarılı kabul edilir.
  return true;
}

void setWritingAddress(const uint8_t *address) {
  if (!address) {
    std::cerr << "HATA: setWritingAddress için geçersiz adres." << std::endl;
    return;
  }
  radio.openWritingPipe(address);
}

bool sendData(const void *packet_data, uint8_t packet_size, bool request_ack) {
  if (!packet_data || packet_size == 0 || packet_size > 32) {
    std::cerr << "HATA: Gönderilecek veri geçersiz veya boyutu hatalı (1-32 "
                 "byte olmalı)."
              << std::endl;
    return false;
  }

  radio.stopListening(); // Göndermeden önce dinlemeyi durdur

  bool report;
  if (request_ack) {
    report = radio.write(packet_data, packet_size);
  } else {
    // ACK istenmiyorsa, writeFast veya NRF24::NO_ACK ile write kullanılabilir
    // Ancak NRF24 kütüphanesindeki TMRh20 fork'unda writeFast'in davranışı
    // değişmiş olabilir. Şimdilik normal write kullanalım, ACK'siz gönderme
    // için kütüphane detaylarına bakmak gerekir. Genellikle autoAck kapatılarak
    // yapılır veya write(data, len, NRF24::NO_ACK) gibi bir seçenek olur.
    // TMRh20's RF24 library: The last parameter of write function (boolean)
    // indicates if it should wait for an ACK or not. So, if request_ack is
    // false, we can use radio.write(packet_data, packet_size, false); However,
    // the function signature in radio.hpp is 'bool sendData(const void*
    // packet_data, uint8_t packet_size, bool request_ack = true);' The
    // 'request_ack' here controls if *we* care about the ACK result for the
    // return value of *this* function, not necessarily if the radio hardware
    // attempts to get an ACK. For now, let's assume radio.write itself handles
    // the ACK attempt if autoAck is on for the pipe. The boolean returned by
    // radio.write() indicates if an ACK was received (if autoAck is enabled for
    // the pipe).
    report = radio.writeFast(packet_data, packet_size);
  }

  radio.startListening(); // Gönderdikten sonra dinlemeye geri dön

  if (!report) {
    // std::cout << "Veri gönderilemedi (ACK alınamadı?)." << std::endl; // Çok
    // fazla log üretebilir
  }
  return report;
}

bool isDataAvailable(uint8_t *pipe_num_available) {
  return radio.available(pipe_num_available);
}

bool readData(void *buffer, uint8_t buffer_max_len, uint8_t &received_len) {
  if (!buffer || buffer_max_len < 32) { // En az 32 byte'lık buffer beklenir
    std::cerr << "HATA: Okuma buffer'ı geçersiz veya çok küçük." << std::endl;
    received_len = 0;
    return false;
  }

  if (radio.available()) {
    received_len = radio.getDynamicPayloadSize();
    if (received_len == 0) { // Payload boyutu alınamadıysa (hata durumu)
      // Bazen 0 dönebilir ve sonraki okumada doğru boyut gelebilir,
      // ya da FIFO boş olabilir. Kütüphane davranışına göre handle etmek
      // gerekebilir. Güvenlik için, eğer 0 ise bir şey okumayalım.
      return false;
    }
    if (received_len > buffer_max_len) {
      std::cerr << "HATA: Alınan veri (" << (int)received_len
                << " byte) buffer boyutundan (" << (int)buffer_max_len
                << " byte) büyük! Veri kaybı olabilir." << std::endl;
      // Veriyi yine de okumayı deneyebiliriz ama sadece buffer_max_len
      // kadarını. Ancak bu genellikle bir sorundur. Şimdilik false dönelim.
      // Veya: radio.read(buffer, buffer_max_len); received_len =
      // buffer_max_len; return true;
      return false;
    }
    radio.read(buffer, received_len);
    return true;
  }
  received_len = 0;
  return false;
}

void printRadioDetails() {
  // Bu fonksiyon RF24 kütüphanesinin `printDetails()` metodunu çağırabilir
  // veya istediğiniz diğer bilgileri yazdırabilir.
  // std::cout << "--- Radyo Detayları ---" << std::endl;
  // radio.printDetails(); // Bu metod genellikle tüm ayarları Serial'e basar.
  // RF24 kütüphanesinin TMRh20 fork'unda printDetails stdout'a
  // yönlendirilebilir. Şimdilik basit bir çıktı verelim:
  if (radio.isChipConnected()) {
    printf("Radio chip connected.\n");
    printf("Channel: %d\n", radio.getChannel());
    printf("Data Rate: %d (0=1MBPS, 1=2MBPS, 2=250KBPS)\n",
           radio.getDataRate());
    printf("PA Level: %d (0=MIN, 1=LOW, 2=HIGH, 3=MAX)\n", radio.getPALevel());
    // Adresleri yazdırmak için ek kod gerekebilir, çünkü RF24::printDetails
    // bunu yapar.
  } else {
    printf("Radio chip not connected.\n");
  }
}
// ... (radio.cpp dosyasının önceki içeriği, printRadioDetails() fonksiyonu
// dahil) ...

// --- Üst Seviye Pakete Özel Gönderme Fonksiyonlarının Gövdeleri ---

bool sendCommandPacket(const CommandPacket &pkt, const uint8_t *target_address,
                       bool request_ack) {
  if (!target_address) {
    std::cerr << "HATA: sendCommandPacket için hedef adres belirtilmedi."
              << std::endl;
    return false;
  }
  // Her gönderme öncesi hedef adresi ayarlamak iyi bir pratiktir,
  // çünkü radyo modülü farklı hedeflere veri gönderebilir.
  setWritingAddress(target_address);
  return sendData(&pkt, sizeof(CommandPacket), request_ack);
}

bool sendTelemetryPacket(const TelemetryPacket &pkt,
                         const uint8_t *target_address, bool request_ack) {
  if (!target_address) {
    std::cerr << "HATA: sendTelemetryPacket için hedef adres belirtilmedi."
              << std::endl;
    return false;
  }
  setWritingAddress(target_address);
  return sendData(&pkt, sizeof(TelemetryPacket), request_ack);
}

bool sendJoinRequestPacket(const JoinRequestPacket &pkt,
                           const uint8_t *target_address, bool request_ack) {
  if (!target_address) {
    std::cerr << "HATA: sendJoinRequestPacket için hedef adres belirtilmedi."
              << std::endl;
    return false;
  }
  setWritingAddress(target_address);
  return sendData(&pkt, sizeof(JoinRequestPacket), request_ack);
}

bool sendJoinResponsePacket(const JoinResponsePacket &pkt,
                            const uint8_t *target_address, bool request_ack) {
  if (!target_address) {
    std::cerr << "HATA: sendJoinResponsePacket için hedef adres belirtilmedi."
              << std::endl;
    return false;
  }
  setWritingAddress(target_address);
  return sendData(&pkt, sizeof(JoinResponsePacket), request_ack);
}

bool sendHeartbeatPacket(const HeartbeatPacket &pkt,
                         const uint8_t *target_address, bool request_ack) {
  if (!target_address) {
    std::cerr << "HATA: sendHeartbeatPacket için hedef adres belirtilmedi."
              << std::endl;
    return false;
  }
  setWritingAddress(target_address);
  return sendData(&pkt, sizeof(HeartbeatPacket), request_ack);
}

bool sendLeaderAnnouncementPacket(const LeaderAnnouncementPacket &pkt,
                                  const uint8_t *target_address,
                                  bool request_ack) {
  if (!target_address) {
    std::cerr
        << "HATA: sendLeaderAnnouncementPacket için hedef adres belirtilmedi."
        << std::endl;
    return false;
  }
  setWritingAddress(target_address);
  return sendData(&pkt, sizeof(LeaderAnnouncementPacket), request_ack);
}

bool sendLeaderRequestPacket(const LeaderRequestPacket &pkt,
                             const uint8_t *target_address, bool request_ack) {
  if (!target_address) {
    std::cerr << "HATA: sendLeaderRequestPacket için hedef adres belirtilmedi."
              << std::endl;
    return false;
  }
  setWritingAddress(target_address);
  return sendData(&pkt, sizeof(LeaderRequestPacket), request_ack);
}
