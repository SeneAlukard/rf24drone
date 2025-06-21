#pragma once
#include <cstdint>
#include <string>

class Mpu6050 {
public:
  Mpu6050() = default;
  ~Mpu6050();

  bool init(const std::string &device = "/dev/i2c-1", uint8_t addr = 0x68);
  bool readAcceleration(int16_t &ax, int16_t &ay, int16_t &az);
  bool readGyro(int16_t &gx, int16_t &gy, int16_t &gz);

private:
  int fd = -1;
  uint8_t address = 0x68;
};
