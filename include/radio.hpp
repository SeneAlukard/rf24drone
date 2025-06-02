#pragma once

#include "../include/packets.hpp" // PacketType ve paket struct'ları için
#include <RF24/RF24.h>            // NRF24L01+ kütüphanesi
#include <cstdint>

// --- Global RF24 Nesnesi ---
// Bu nesne bir .cpp dosyasında (örn: radio.cpp veya main.cpp) tanımlanmalıdır.
// Örnek tanım: RF24 radio(CE_PINI, CSN_PINI); (Pin numaraları uygun şekilde
// verilmeli)
extern RF24 radio;

// --- RF Adresleme Ayarları ---
constexpr uint8_t RF24_ADDRESS_WIDTH =
    5; // NRF24L01+ adres genişliği (3, 4 veya 5 byte olabilir)

// Örnek RF adresleri (bir .cpp dosyasında tanımlanmaları gerekir)
// extern const uint8_t GROUND_STATION_ADDRESS[RF24_ADDRESS_WIDTH];
// extern const uint8_t DRONE_BROADCAST_ADDRESS[RF24_ADDRESS_WIDTH];
// extern uint8_t DRONE_INDIVIDUAL_BASE_ADDRESS[RF24_ADDRESS_WIDTH - 1]; //
// Dinamik atanacak pipe'lar için temel adres

/**
 * @brief RF24 radyo modülünü başlatır ve temel ayarları yapar.
 *
 * Haberleşme kanalı, veri hızı, güç seviyesi gibi parametreleri ayarlar.
 * Başlangıç dinleme ve yazma pipe'larını açar.
 *
 * @param rf_channel Kullanılacak RF kanalı (0-125).
 * @param my_listen_address_p0 Bu cihazın dinleyeceği ana adres (Pipe 0 için).
 * @param my_listen_address_p1 Bu cihazın dinleyeceği ikincil adres (Pipe 1
 * için, opsiyonel).
 * @return Başlatma başarılıysa true, aksi takdirde false.
 */
bool setupRFCommunication(
    uint8_t rf_channel, const uint8_t *my_listen_address_p0,
    const uint8_t
        *my_listen_address_p1); // Diğer dinleme pipe'ları da eklenebilir

/**
 * @brief Belirtilen pipe numarasında (1-5) dinleme için yeni bir adres açar.
 * Pipe 0 `setupRFCommunication` içinde ayarlanır.
 * @param pipe_number Açılacak pipe numarası (1-5).
 * @param address Bu pipe için dinlenecek adres.
 * @return Başarılıysa true, değilse false.
 */
bool openListeningPipe(uint8_t pipe_number, const uint8_t *address);

/**
 * @brief Yazma yapılacak adresi ayarlar.
 * @param address Yazma yapılacak hedef adres.
 */
void setWritingAddress(const uint8_t *address);

/**
 * @brief Veri paketini gönderir.
 *
 * Bu genel bir gönderme fonksiyonudur. `packet_data` işaretçisi,
 * gönderilecek olan serileştirilmiş paketi (örn: packets.hpp'deki bir struct)
 * içermelidir. RF24 kütüphanesindeki `write` fonksiyonu varsayılan olarak
 * blocklayıcıdır ancak ayarlanabilir.
 *
 * @param packet_data Gönderilecek verinin işaretçisi.
 * @param packet_size Verinin boyutu (byte cinsinden, NRF24L01+ için maks 32
 * byte).
 * @param request_ack true ise ACK ile gönderim denenir (NRF24'ün ACK özelliğini
 * kullanır).
 * @return Veri gönderildiyse ve (eğer request_ack true ise) ACK alındıysa true,
 * aksi halde false.
 */
bool sendData(const void *packet_data, uint8_t packet_size,
              bool request_ack = true);

/**
 * @brief Herhangi bir pipe'tan okunacak veri olup olmadığını kontrol eder.
 *
 * @param pipe_num_available Eğer veri varsa, verinin geldiği pipe numarasının
 * (0-5) yazılacağı byte işaretçisi.
 * @return Veri varsa true, yoksa false.
 */
bool isDataAvailable(uint8_t *pipe_num_available);

/**
 * @brief Gelen veriyi sağlanan buffer'a okur.
 *
 * FIFO'dan bir paket okur. Öncesinde isDataAvailable() çağrılmalıdır.
 *
 * @param buffer Alınan verinin depolanacağı buffer işaretçisi.
 * @param buffer_max_len Buffer'ın maksimum boyutu (en az 32 byte olmalı).
 * @param received_len Alınan verinin gerçek boyutunun yazılacağı referans.
 * @return Veri başarıyla okunduysa true, aksi halde false.
 */
bool readData(void *buffer, uint8_t buffer_max_len, uint8_t &received_len);

/**
 * @brief Radyo modülünün mevcut ayarlarını ve durumunu yazdırmak için yardımcı
 * fonksiyon (debug amaçlı). Bu fonksiyonun implementasyonu için <iostream> gibi
 * kütüphaneler gerekebilir (.cpp dosyasında).
 */
void printRadioDetails();

// --- Üst Seviye Pakete Özel Gönderme Fonksiyonları ---
// Bu blok yalnızca bir kez bulunmalıdır.

/**
 * @brief Bir CommandPacket gönderir.
 * @param pkt Gönderilecek CommandPacket.
 * @param target_address Hedef cihazın RF adresi.
 * @param request_ack Bu paket için ACK istenip istenmediği (varsayılan: true).
 * @return Gönderme başarılı olduysa (ve ACK istendiyse alındıysa) true, aksi
 * halde false.
 */
bool sendCommandPacket(const CommandPacket &pkt, const uint8_t *target_address,
                       bool request_ack = true);

/**
 * @brief Bir TelemetryPacket gönderir.
 * @param pkt Gönderilecek TelemetryPacket.
 * @param target_address Hedef cihazın RF adresi (örn: yer istasyonu/lider).
 * @param request_ack Bu paket için ACK istenip istenmediği (telemetri için
 * genellikle false).
 * @return Gönderme başarılı olduysa (ve ACK istendiyse alındıysa) true, aksi
 * halde false.
 */
bool sendTelemetryPacket(const TelemetryPacket &pkt,
                         const uint8_t *target_address,
                         bool request_ack = false);

/**
 * @brief Bir JoinRequestPacket gönderir.
 * @param pkt Gönderilecek JoinRequestPacket.
 * @param target_address Hedef cihazın RF adresi (örn: yer istasyonu/bilinen
 * keşif adresi).
 * @param request_ack Bu paket için ACK istenip istenmediği (varsayılan: true).
 * @return Gönderme başarılı olduysa (ve ACK istendiyse alındıysa) true, aksi
 * halde false.
 */
bool sendJoinRequestPacket(const JoinRequestPacket &pkt,
                           const uint8_t *target_address,
                           bool request_ack = true);

/**
 * @brief Bir JoinResponsePacket gönderir.
 * @param pkt Gönderilecek JoinResponsePacket.
 * @param target_address Hedef cihazın RF adresi (JoinRequest gönderen drone'un
 * adresi).
 * @param request_ack Bu paket için ACK istenip istenmediği (varsayılan: true).
 * @return Gönderme başarılı olduysa (ve ACK istendiyse alındıysa) true, aksi
 * halde false.
 */
bool sendJoinResponsePacket(const JoinResponsePacket &pkt,
                            const uint8_t *target_address,
                            bool request_ack = true);

/**
 * @brief Bir HeartbeatPacket gönderir.
 * @param pkt Gönderilecek HeartbeatPacket.
 * @param target_address Hedef cihazın RF adresi (örn: yer istasyonu/lider veya
 * broadcast).
 * @param request_ack Bu paket için ACK istenip istenmediği (heartbeat için
 * genellikle false).
 * @return Gönderme başarılı olduysa (ve ACK istendiyse alındıysa) true, aksi
 * halde false.
 */
bool sendHeartbeatPacket(const HeartbeatPacket &pkt,
                         const uint8_t *target_address,
                         bool request_ack = false);

/**
 * @brief Bir LeaderAnnouncementPacket gönderir.
 * @param pkt Gönderilecek LeaderAnnouncementPacket.
 * @param target_address Hedef cihazın RF adresi (genellikle bir broadcast
 * adresi).
 * @param request_ack Bu paket için ACK istenip istenmediği (duyurular için
 * genellikle false).
 * @return Gönderme başarılı olduysa (ve ACK istendiyse alındıysa) true, aksi
 * halde false.
 */
bool sendLeaderAnnouncementPacket(const LeaderAnnouncementPacket &pkt,
                                  const uint8_t *target_address,
                                  bool request_ack = false);

/**
 * @brief Bir LeaderRequestPacket gönderir.
 * @param pkt Gönderilecek LeaderRequestPacket.
 * @param target_address Hedef cihazın RF adresi (örn: yer istasyonu/mevcut
 * lider).
 * @param request_ack Bu paket için ACK istenip istenmediği (varsayılan: true).
 * @return Gönderme başarılı olduysa (ve ACK istendiyse alındıysa) true, aksi
 * halde false.
 */
bool sendLeaderRequestPacket(const LeaderRequestPacket &pkt,
                             const uint8_t *target_address,
                             bool request_ack = true);
