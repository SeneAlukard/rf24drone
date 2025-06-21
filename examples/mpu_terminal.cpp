#include "mpu6050.hpp"
#include <chrono>
#include <iostream>
#include <thread>

int main() {
    Mpu6050 sensor;
    if (!sensor.init()) {
        std::cerr << "Failed to initialize MPU6050" << std::endl;
        return 1;
    }

    std::cout << "MPU6050 initialized. Reading data..." << std::endl;
    while (true) {
        int16_t ax, ay, az;
        int16_t gx, gy, gz;
        if (sensor.readAcceleration(ax, ay, az) && sensor.readGyro(gx, gy, gz)) {
            std::cout << "AX: " << ax << " AY: " << ay << " AZ: " << az
                      << " | GX: " << gx << " GY: " << gy << " GZ: " << gz
                      << std::endl;
        } else {
            std::cerr << "Sensor read error" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}
