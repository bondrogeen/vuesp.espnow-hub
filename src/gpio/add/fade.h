#ifndef FADE_H
#define FADE_H

#include <Arduino.h>
#include <math.h>

#include <vector>

#define PORT_READ  0
#define PORT_WRITE 1

typedef bool (*PortFadeProvider)(uint8_t gpio, uint8_t action, uint16_t& value);
typedef void (*StateFadeChangeProvider)(uint8_t gpio, uint16_t value);

class Fade {
 private:
  struct Channel {
    uint8_t gpio;
    uint16_t targetValue;
    uint16_t currentValue;
    uint16_t startValue;
    unsigned long startTime;
    unsigned long duration;
    uint8_t gamma;
    bool active;
    bool initialized;
    bool firstLoop;
    Channel();
  };

  std::vector<Channel> channels;
  PortFadeProvider portProvider;
  StateFadeChangeProvider stateChangeProvider;
  uint8_t maxChannels;
  uint8_t defaultGamma;

  bool readPortValue(uint8_t gpio, uint16_t& value);
  uint16_t gammaCorrection(uint16_t value, uint8_t gamma);

 public:
  Fade();
  void init(uint8_t maxChannelsCount, uint8_t gamma = 3);
  void setDataProvider(PortFadeProvider provider);
  void setStateChangeProvider(StateFadeChangeProvider provider);
  bool start(uint8_t gpio, uint16_t targetValue, unsigned long duration);
  bool start(uint8_t gpio, uint16_t targetValue, unsigned long duration, uint8_t gamma);
  bool stop(uint8_t gpio);
  bool isActive(uint8_t gpio);
  uint16_t getCurrentValue(uint8_t gpio);
  void loop();
};

#endif