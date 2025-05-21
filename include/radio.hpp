#pragma once
#include <cstdint>
#include <iostream>
#include <unistd.h>

// ----------- USER DEFINED INCLUDES ----------- //
#include "packets.hpp"

// ----------- DOWNLOANDED LIBRARY INCLUDES ----------- //
#include <RF24/RF24.h>

extern RF24 radio;

void setupRFCommunication();
void sendCommandAndReceive(uint8_t ID, const uint8_t *drone_addr);
