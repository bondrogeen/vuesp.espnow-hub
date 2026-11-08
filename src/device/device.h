#ifndef DEVICE_H
#define DEVICE_H

#include <Wire.h>

#include "../libs/UnixTime.h"
#include "./gpio/gpio.h"
#include "./runner/runner.h"


void setupDevice();
void setupFirstDevice();
void deviceGPIO(Port* port, uint8_t type);
void loopDevice(uint32_t now);
void onWsEventDevice(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task);

#endif