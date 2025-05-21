#pragma once
#include <cstdint>

struct CommandPacket {
  uint8_t drone_id;
  char command[20];
};

struct TelemetryPacket {
  uint8_t droneID;
  float altitude;
  float battery;
};
