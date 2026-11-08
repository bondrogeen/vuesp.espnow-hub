#ifndef GPIO_H
#define GPIO_H

#define GPIO_MODE_INPUT INPUT
#define GPIO_MODE_INPUT_PULLUP INPUT_PULLUP
#define GPIO_MODE_OUTPUT OUTPUT
#define GPIO_MODE_OUTPUT_OPEN_DRAIN OUTPUT_OPEN_DRAIN
#define GPIO_MODE_PWM 128
#define GPIO_MODE_ONEWIRE 129
#define GPIO_MODE_ADC 130
#define GPIO_MODE_DAC 131

#define GPIO_EVENT_OFF 0
#define GPIO_EVENT_ON 1

#define GPIO_DISABLED_OFF 0
#define GPIO_DISABLED_ON 1

#define GPIO_INTERRUPT_OFF 0
#define GPIO_INTERRUPT_RISING 1
#define GPIO_INTERRUPT_FALLING 2
#define GPIO_INTERRUPT_CHANGE 3

#define GPIO_COMMAND_SAVE 254
#define GPIO_COMMAND_GET 1
#define GPIO_COMMAND_SET 2
#define GPIO_COMMAND_GET_ALL 3
#define GPIO_COMMAND_CHANGE 4
#define GPIO_COMMAND_RESTORE 5

#define GPIO_BIT_INPUT (1 << 0)              // 0b0000000000000001
#define GPIO_BIT_INPUT_PULLUP (1 << 1)       // 0b0000000000000010
#define GPIO_BIT_OUTPUT (1 << 2)             // 0b0000000000000100
#define GPIO_BIT_OUTPUT_OPEN_DRAIN (1 << 3)  // 0b0000000000001000
#define GPIO_BIT_PWM (1 << 4)                // 0b0000000000010000
#define GPIO_BIT_ONEWIRE (1 << 5)            // 0b0000000000100000
#define GPIO_BIT_ADC (1 << 6)                // 0b0000000001000000
#define GPIO_BIT_DAC (1 << 7)                // 0b0000000010000000

#define GPIO_BIT_ALL_NOT_DAC (GPIO_BIT_INPUT | GPIO_BIT_INPUT_PULLUP | GPIO_BIT_OUTPUT | GPIO_BIT_OUTPUT_OPEN_DRAIN | GPIO_BIT_PWM | GPIO_BIT_ONEWIRE | GPIO_BIT_ADC)  // 0b0000000001111111
#define GPIO_BIT_ALL_NOT_DAC_AND_ADC (GPIO_BIT_INPUT | GPIO_BIT_INPUT_PULLUP | GPIO_BIT_OUTPUT | GPIO_BIT_OUTPUT_OPEN_DRAIN | GPIO_BIT_PWM | GPIO_BIT_ONEWIRE)         // 0b0000000001111111

#define LONG_PRESS_TIME 1000
#define REPEAT_START_TIME 2000
#define REPEAT_INTERVAL 100
#define CLICK_WINDOW 200

#define EVENT_NONE 0
#define EVENT_LONG_PRESS 1
#define EVENT_REPEAT 2
#define EVENT_CLICK 3

#include <OneWire.h>

#include "../init/init.h"
#include "./add/fade.h"
#include "./webserver/webserver.h"

void setupGPIO();
void setupFirstGPIO();
void loopGPIO(uint32_t now);
uint16_t getValue(uint8_t gpio, uint16_t& value);
void setValue(uint8_t gpio, uint16_t value);
void setValue(uint8_t gpio, uint16_t value, uint8_t mode);
void updatePort(uint8_t gpio, uint16_t value);

extern Port ports[];
extern int ports_len;

#endif