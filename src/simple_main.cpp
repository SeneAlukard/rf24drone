#include "mpu6050.hpp"
#include "radio.hpp"
#include <chrono>
#include <cstring>
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

int main() {
  uint8_t drone_id = 1; // sabit ID
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
  std::uniform_int_distribution<int> jitter(50, 100);
  std::uniform_int_distribution<int16_t> randGyro(-100, 100);
  std::uniform_int_distribution<int16_t> randAccel(-100, 100);

  SimplePacket pkt{};
  pkt.drone_id = drone_id;

  while (true) {
    int16_t ax = 0, ay = 0, az = 0;
    int16_t gx = 0, gy = 0, gz = 0;
    bool ok = sensor.readAcceleration(ax, ay, az) && sensor.readGyro(gx, gy, gz);
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

    char buf[32] = {0};
    if (radio.receive(buf, sizeof(buf), true)) {
      buf[31] = '\0';
      radio.receive(buf, sizeof(buf));

      unsigned int id = 0;
      char state[6] = {0};
      if (std::sscanf(buf, "%u Leader = %5s", &id, state) == 2) {
        bool val = std::strcmp(state, "True") == 0 || std::strcmp(state, "true") == 0;
        if (id == drone_id)
          is_leader = val;
        else
          is_leader = false;
      } else if (std::sscanf(buf, "LEADER = %u", &id) == 1) {
        is_leader = (id == drone_id);
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  return 0;
}
