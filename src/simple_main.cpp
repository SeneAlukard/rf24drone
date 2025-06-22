#include "mpu6050.hpp"
#include "radio.hpp"
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <random>
#include <thread>

#define TX_CE_PIN 22
#define TX_CSN_PIN 10
#define RX_CE_PIN 27
#define RX_CSN_PIN 0
static constexpr uint64_t BASE_TX = 0xF0F0F0F0D2ULL;
static constexpr uint64_t BASE_RX = 0xF0F0F0F0E1ULL;

struct SimplePacket {
  uint8_t drone_id;
  int16_t accel_x;
  int16_t accel_y;
  int16_t accel_z;
  int16_t gyro_x;
  int16_t gyro_y;
  int16_t gyro_z;
  bool leader;
};

int main(int argc, char **argv) {
  uint8_t drone_id = 1;
  if (argc > 1) {
    int id = std::atoi(argv[1]);
    if (id > 0 && id < 255)
      drone_id = static_cast<uint8_t>(id);
  }
  bool is_leader = false;

  RadioInterface radio(TX_CE_PIN, TX_CSN_PIN);
  if (!radio.begin()) {
    std::cerr << "Radio başlatılamadı\n";
    return 1;
  }

  radio.configure(1, RadioDataRate::MEDIUM_RATE);
  radio.setAddress(BASE_TX, BASE_RX);

  Mpu6050 sensor;
  if (!sensor.init()) {
    std::cerr << "MPU6050 başlatılamadı, rasgele gyro kullanılacak\n";
  }

  std::default_random_engine rng{static_cast<unsigned>(std::time(nullptr))};
  std::uniform_int_distribution<int> slot_offset(0, 100);
  std::uniform_int_distribution<int16_t> randGyro(-100, 100);
  std::uniform_int_distribution<int16_t> randAccel(-100, 100);

  SimplePacket pkt{};
  pkt.drone_id = drone_id;

  std::chrono::steady_clock::time_point send_start;
  std::chrono::steady_clock::time_point send_end;
  bool have_sync = false;
  bool sent = false;

  while (true) {
    char buf[32] = {0};
    if (radio.receive(buf, sizeof(buf), true)) {
      buf[31] = '\0';
      radio.receive(buf, sizeof(buf));
      if (std::strcmp(buf, "SYNC") == 0) {
        auto now = std::chrono::steady_clock::now();
        int offset = slot_offset(rng);
        if (drone_id == 1)
          send_start = now + std::chrono::milliseconds(offset);
        else
          send_start = now + std::chrono::milliseconds(100 + offset);
        send_end = send_start + std::chrono::milliseconds(100);
        have_sync = true;
        sent = false;
        continue;
      }

      unsigned int id = 0;
      if (std::sscanf(buf, "%u Leader = True", &id) == 1 && id == drone_id) {
        is_leader = true;
      }
    }

    auto now = std::chrono::steady_clock::now();
    if (have_sync && !sent && now >= send_start && now < send_end) {
      int16_t ax = 0, ay = 0, az = 0;
      int16_t gx = 0, gy = 0, gz = 0;
      bool ok = sensor.readAcceleration(ax, ay, az) &&
                 sensor.readGyro(gx, gy, gz);
      if (!ok) {
        ax = randAccel(rng);
        ay = randAccel(rng);
        az = randAccel(rng);
        gx = randGyro(rng);
        gy = randGyro(rng);
        gz = randGyro(rng);
      }
      pkt.accel_x = ax;
      pkt.accel_y = ay;
      pkt.accel_z = az;
      pkt.gyro_x = gx;
      pkt.gyro_y = gy;
      pkt.gyro_z = gz;
      pkt.leader = is_leader;
      radio.send(&pkt, sizeof(pkt));
      sent = true;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

  return 0;
}
