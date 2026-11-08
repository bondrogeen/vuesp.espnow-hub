#include "fade.h"

Fade::Channel::Channel() : gpio(0), targetValue(0), currentValue(0), startValue(0), 
                           startTime(0), duration(0), gamma(3),
                           active(false), initialized(false), firstLoop(true) {}

Fade::Fade() : portProvider(nullptr), stateChangeProvider(nullptr), maxChannels(0), defaultGamma(3) {}

void Fade::init(uint8_t maxChannelsCount, uint8_t gamma) {
    maxChannels = maxChannelsCount;
    defaultGamma = gamma;
    channels.resize(maxChannelsCount);
    for (uint8_t i = 0; i < maxChannelsCount; i++) {
        channels[i] = Channel();
        channels[i].gamma = gamma;
    }
}

void Fade::setDataProvider(PortFadeProvider provider) {
    portProvider = provider;
}

void Fade::setStateChangeProvider(StateFadeChangeProvider provider) {
    stateChangeProvider = provider;
}

bool Fade::readPortValue(uint8_t gpio, uint16_t& value) {
    return portProvider ? portProvider(gpio, PORT_READ, value) : false;
}

uint16_t Fade::gammaCorrection(uint16_t value, uint8_t gamma) {
    if (value == 0) return 0;
    if (value == 255) return 255;
    
    float normalized = (float)value / 255.0f;
    float corrected = powf(normalized, (float)gamma);
    return (uint16_t)(corrected * 255.0f + 0.5f);
}

bool Fade::start(uint8_t gpio, uint16_t targetValue, unsigned long duration) {
    return start(gpio, targetValue, duration, defaultGamma);
}

bool Fade::start(uint8_t gpio, uint16_t targetValue, unsigned long duration, uint8_t gamma) {
    if (!portProvider) return false;

    int8_t freeChannel = -1, existingChannel = -1;
    for (uint8_t i = 0; i < channels.size(); i++) {
        if (!channels[i].active && freeChannel == -1) freeChannel = i;
        if (channels[i].gpio == gpio && channels[i].active) {
            existingChannel = i;
            break;
        }
    }

    if (existingChannel != -1) {
        Channel& ch = channels[existingChannel];
        ch.startValue = ch.currentValue;
        ch.targetValue = targetValue;
        ch.duration = duration;
        ch.gamma = gamma;
        ch.startTime = millis();
        ch.active = true;
        ch.firstLoop = true;
        return true;
    }
    
    if (freeChannel == -1) return false;

    Channel& ch = channels[freeChannel];
    ch.gpio = gpio;
    ch.targetValue = targetValue;
    ch.duration = duration;
    ch.gamma = gamma;
    ch.active = true;
    ch.initialized = true;
    ch.startTime = millis();
    ch.firstLoop = true;

    uint16_t currentVal;
    if (readPortValue(gpio, currentVal)) {
        if (currentVal > 255) currentVal = 255;
        ch.currentValue = gammaCorrection(currentVal, gamma);
        ch.startValue = ch.currentValue;
    } else {
        ch.currentValue = 0;
        ch.startValue = 0;
    }
    
    return true;
}

bool Fade::stop(uint8_t gpio) {
    for (uint8_t i = 0; i < channels.size(); i++) {
        if (channels[i].gpio == gpio && channels[i].active) {
            channels[i].active = false;
            return true;
        }
    }
    return false;
}

bool Fade::isActive(uint8_t gpio) {
    for (uint8_t i = 0; i < channels.size(); i++) {
        if (channels[i].gpio == gpio && channels[i].active) return true;
    }
    return false;
}

uint16_t Fade::getCurrentValue(uint8_t gpio) {
    for (uint8_t i = 0; i < channels.size(); i++) {
        if (channels[i].gpio == gpio) return channels[i].currentValue;
    }
    return 0;
}

void Fade::loop() {
    unsigned long currentTime = millis();
    for (uint8_t i = 0; i < channels.size(); i++) {
        Channel& ch = channels[i];
        if (!ch.active || !ch.initialized) continue;

        if (ch.firstLoop) {
            ch.firstLoop = false;
            ch.startTime = currentTime;
            if (portProvider) {
                uint16_t temp = ch.startValue;
                portProvider(ch.gpio, PORT_WRITE, temp);
            }
            continue;
        }

        unsigned long elapsed = currentTime - ch.startTime;
        
        if (elapsed >= ch.duration) {
            ch.active = false;
            uint16_t finalValue = gammaCorrection(ch.targetValue, ch.gamma);
            if (ch.currentValue != finalValue) {
                ch.currentValue = finalValue;
                if (portProvider) {
                    uint16_t temp = finalValue;
                    portProvider(ch.gpio, PORT_WRITE, temp);
                }
            }
            if (stateChangeProvider) {
                stateChangeProvider(ch.gpio, ch.targetValue);
            }
            continue;
        }

        float progress = (float)elapsed / ch.duration;
        
        uint16_t linearValue;
        if (ch.startValue <= ch.targetValue) {
            linearValue = ch.startValue + (ch.targetValue - ch.startValue) * progress;
        } else {
            linearValue = ch.startValue - (ch.startValue - ch.targetValue) * progress;
        }
        if (linearValue > 255) linearValue = 255;
        
        uint16_t gammaValue = gammaCorrection(linearValue, ch.gamma);
        
        if (gammaValue != ch.currentValue) {
            ch.currentValue = gammaValue;
            if (portProvider) {
                uint16_t temp = gammaValue;
                portProvider(ch.gpio, PORT_WRITE, temp);
            }
        }
    }
}