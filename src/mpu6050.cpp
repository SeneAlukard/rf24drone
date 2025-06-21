#include "mpu6050.hpp"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <cstdint>
#include <cstring>

Mpu6050::~Mpu6050() {
  if (fd >= 0) {
    close(fd);
  }
}

bool Mpu6050::init(const std::string &device, uint8_t addr) {
  address = addr;
  fd = open(device.c_str(), O_RDWR);
  if (fd < 0)
    return false;
  if (ioctl(fd, I2C_SLAVE, address) < 0) {
    close(fd);
    fd = -1;
    return false;
  }
  // Wake up the device by clearing sleep bit in PWR_MGMT_1
  uint8_t buf[2] = {0x6B, 0x00};
  if (write(fd, buf, 2) != 2) {
    close(fd);
    fd = -1;
    return false;
  }
  return true;
}

static bool readRegisters(int fd, uint8_t reg, uint8_t *data, size_t len) {
  if (write(fd, &reg, 1) != 1)
    return false;
  if (read(fd, data, len) != static_cast<int>(len))
    return false;
  return true;
}

bool Mpu6050::readAcceleration(int16_t &ax, int16_t &ay, int16_t &az) {
  if (fd < 0)
    return false;
  uint8_t data[6];
  if (!readRegisters(fd, 0x3B, data, sizeof(data)))
    return false;
  ax = static_cast<int16_t>((data[0] << 8) | data[1]);
  ay = static_cast<int16_t>((data[2] << 8) | data[3]);
  az = static_cast<int16_t>((data[4] << 8) | data[5]);
  return true;
}

bool Mpu6050::readGyro(int16_t &gx, int16_t &gy, int16_t &gz) {
  if (fd < 0)
    return false;
  uint8_t data[6];
  if (!readRegisters(fd, 0x43, data, sizeof(data)))
    return false;
  gx = static_cast<int16_t>((data[0] << 8) | data[1]);
  gy = static_cast<int16_t>((data[2] << 8) | data[3]);
  gz = static_cast<int16_t>((data[4] << 8) | data[5]);
  return true;
}
