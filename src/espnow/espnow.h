#pragma once

#include <Arduino.h>

void espnowSetup();
void espnowLoop(uint32_t now);
const char* espnowHubId();
