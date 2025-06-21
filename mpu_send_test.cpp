#include "radio.hpp"
#include "mpu6050.hpp"
#include "packets.hpp"
#include <chrono>
#include <cstring>
#include <ctime>
#include <iostream>
#include <thread>

#define TX_CE_PIN 27
#define TX_CSN_PIN 0
#define RX_CE_PIN 22
#define RX_CSN_PIN 10

static constexpr uint64_t GBS_ADDR_TX = 0xF0F0F0F0D2ULL;
static constexpr uint64_t GBS_ADDR_RX = 0xF0F0F0F0E1ULL;

int main() {
    RadioInterface radio(TX_CE_PIN, TX_CSN_PIN, RX_CE_PIN, RX_CSN_PIN);
    Mpu6050 sensor;

    if (!sensor.init()) {
        std::cerr << "Failed to init MPU6050. Using dummy data." << std::endl;
    }

    if (!radio.begin()) {
        std::cerr << "Failed to initialize radio" << std::endl;
        return 1;
    }

    radio.configure(90, RadioDataRate::MEDIUM_RATE);
    radio.setAddress(GBS_ADDR_TX, GBS_ADDR_RX);

    for (int i = 0; i < 10; ++i) {
        int16_t ax = 0, ay = 0, az = 0;
        int16_t gx = 0, gy = 0, gz = 0;
        bool ok = sensor.readAcceleration(ax, ay, az) && sensor.readGyro(gx, gy, gz);
        if (!ok) {
            ax = ay = az = 0;
            gx = gy = gz = 0;
        }

        TelemetryPacket pkt{};
        pkt.drone_id = 1;
        pkt.timestamp = static_cast<uint32_t>(std::time(nullptr));
        pkt.acceleration_x = ax;
        pkt.acceleration_y = ay;
        pkt.acceleration_z = az;
        pkt.gyroscope_x = gx;
        pkt.gyroscope_y = gy;
        pkt.gyroscope_z = gz;
        pkt.battery_voltage = 3.7f;
        pkt.altitude = 0.0f;
        pkt.rpd = radio.testRPD() ? 1 : 0;
        pkt.retries = 0;
        pkt.link_quality = 0.0f;

        radio.send(&pkt, sizeof(pkt));
        std::cout << "Telemetry sent" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}
