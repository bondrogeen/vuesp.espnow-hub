#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <WiFiUdp.h>

#include "./init/init.h"
#include "./protocol/DeviceProtocol.h"
#include "./runner/runner.h"
#include "./webserver/webserver.h"

#define UDP_IP {255, 255, 255, 255}

void setupDiscovery();
void loopDiscovery(uint32_t now);
void commDiscovery();
extern DeviceProtocol protocol;
extern Discovery discovery;

#endif