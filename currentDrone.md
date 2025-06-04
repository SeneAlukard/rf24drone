# RF24 Tabanlı Dağıtık Drone Ağı – Sistem Tasarımı

## 🎯 Amaç

Bu sistem, nRF24L01 donanımıyla çalışan, kimliği atanmış droneların:
- Yer istasyonuna bağlanması
- Komutları lider drone üzerinden alması
- Sensör verilerini göndermesi
- Lider kaybını algılaması ve yeniden iletişime geçmesi
üzerine kurulmuştur.

---

## 📡 RF Kanal Yapısı

| Kanal     | Amaç                                     |
|-----------|------------------------------------------|
| Channel 1 | Başlangıç (Join) kanalı |
| Atanan Kanal | Drone iletişimi ve sensör uplink |

---

## 🧱 Drone İç Yapısı

### Temel Alanlar

- `drone_id: uint8_t` – Drone'un atanmış kimliği
- `leader_id: std::optional<uint8_t>` – Güncel liderin kimliği
- `last_heartbeat_time: uint32_t` – Son alınan lider paketi
- `std::queue<Packet> rxQueue` – Alınan ama henüz işlenmemiş paketler

---

## 🧠 Davranış Akışı

1. Drone varsayılan `Channel 1` üzerinden yer istasyonuna `JoinRequest` yollar
2. Yer istasyonu:
   - `JoinResponse` (drone_id + leader_id + channel) yollar
3. Drone verilen kanala (`JoinResponse.channel`) geçer ve ana döngü başlar:
   - RF'den paket toplar (`rxQueue`)
   - Komut varsa:
     - Eğer `target_id == drone_id` ve `gecikme ≤ 3000ms` ise uygular
     - Eğer `next_leader_id` içeriyorsa lider güncellenir
     - Eğer liderse, yer istasyonundan gelen komutları yayınlar
   - Belirli aralıklarla sensör verisi gönderilir (atanan kanal)
   - 5 saniyeden uzun süre lider mesajı gelmezse:
     - `leader_id = std::nullopt`
     - Yer istasyonuna `LeaderRequest` gönderilir

---

## 📦 Paket Tipleri (`PacketType`)

- `JoinRequest`
- `JoinResponse { drone_id, leader_id, channel }`
- `CommandPacket { target_id, command, payload, timestamp, next_leader_id? }`
- `SensorPacket`
- `Heartbeat`
- `LeaderAnnouncement { new_leader_id }`
- `LeaderRequest`

---

## 🔐 Güvenlik

- Tüm mesajlar AES-128 ile şifrelenir (shared key)
- CRC veya ek imzaya gerek kalmaz

---

## ⏱️ Zaman Duyarlılığı

- `CommandPacket` içinde `timestamp: uint32_t` bulunur
- Eğer komut `> 3000ms` gecikmişse drone tarafından yoksayılır

---

## ❤️ Heartbeat Sistemi

- Her lider periyodik `Heartbeat` yayar (varsayılan: 2s)
- Ya da düzenli `CommandPacket`’ler heartbeat yerine sayılır
- Diğer drone'lar en son gelen lider mesajının zamanını tutar

---

## 🔁 Lider Değişimi

- Yer istasyonu yeni lideri `LeaderAnnouncement` ile bildirir
- Mevcut lider farkındaysa, `CommandPacket.next_leader_id` ile el değişimini iletir
- Eski lider artık normal drone moduna geçer

---

## 📥 Paket Kuyruğu

- `radio.available()` süresince tüm paketler `rxQueue`'ya eklenir
- FIFO işlenir, hiçbir mesaj kaybolmaz

---

## 🛑 RF24 Half-Duplex Uyum

- `sendPacket()` içinde:
  - RX doluysa önce gelen alınır
  - RX boşalana kadar beklenir
  - Sonra TX yapılır ve RX tekrar açılır

## 📨 Telemetri Gönderim İzni

- Tam çift yönlü haberleşme imkanımız olmadığından,
  drone'lar telemetri paketlerini ancak yer istasyonundan
  gelen `PermissionToSend` paketinden sonra yollar.
- Bu mekanizma TX/RX çakışmasını önler.

---

## ❌ Bilinçli Olarak Eklenmeyenler

| Özellik     | Neden                       |
|-------------|-----------------------------|
| OTA Güncelleme | Donanımsal risk + düşük öncelik |
| ACK / Retry | İletim güvenliği için gerekirse sonra eklenebilir |
| Yedek lider | Gerekirse yer istasyonu dinamik atar |

---

## 📦 Genişletme Potansiyeli

- Paket imzalama (HMAC, RSA) eklenebilir
- Uzak log sistemi (yer istasyonuna hata akışı)
- RF24'ün dışına çıkılarak LoRa gibi alternatiflere geçilebilir

---

## 🧭 Durum Özeti

Sistem şu özellikleri karşılamaktadır:

- ✅ Komut yönlendirme (lider)
- ✅ Zaman senkronizasyonu
- ✅ Komut filtreleme (gecikme tabanlı)
- ✅ Şifreli iletişim
- ✅ Dinamik lider değişimi
- ✅ Kaybolmayan mesaj işleme (FIFO)


