#include "radio.hpp"
#include "mpu6050.hpp"
#include <chrono>
#include <cstring>
#include <iostream>
#include <random>
#include <thread>

#define TX_CE_PIN 27
#define TX_CSN_PIN 0
#define RX_CE_PIN 22
#define RX_CSN_PIN 10
static constexpr uint64_t BASE_TX = 0xF0F0F0F0D2ULL;
static constexpr uint64_t BASE_RX = 0xF0F0F0F0E1ULL;

struct SimplePacket {
    uint8_t drone_id;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
    bool leader;
};

int main() {
    uint8_t drone_id = 1; // sabit ID
    bool is_leader = false;

    RadioInterface radio(TX_CE_PIN, TX_CSN_PIN, RX_CE_PIN, RX_CSN_PIN);
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

    SimplePacket pkt{};
    pkt.drone_id = drone_id;

    while (true) {
        int16_t gx=0, gy=0, gz=0;
        bool ok = sensor.readGyro(gx, gy, gz);
        if (!ok) {
            gx = randGyro(rng);
            gy = randGyro(rng);
            gz = randGyro(rng);
        }
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
            if (std::sscanf(buf, "%u Leader = True", &id) == 1 && id == drone_id) {
                is_leader = true;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(jitter(rng)));
    }

    return 0;
}

