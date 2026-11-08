#ifndef ARDUINO_STUB_H
#define ARDUINO_STUB_H

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cctype>

typedef uint8_t byte;

#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define PORT_TYPE_DIGITAL 0
#define PORT_TYPE_PWM 1

extern uint32_t stub_millis;

inline uint32_t millis() { return stub_millis; }
inline void delay(uint32_t ms) { stub_millis += ms; }

inline long random(long max) { return rand() % max; }
inline long random(long min, long max) { return min + rand() % (max - min); }

inline int analogRead(uint8_t pin) { (void)pin; return 0; }

class StubSerial {
public:
    void begin(unsigned long) {}
    void print(const char*) {}
    void print(int) {}
    void print(unsigned int) {}
    void print(long) {}
    void print(unsigned long) {}
    void print(double) {}
    void println(const char*) {}
    void println(int) {}
    void println(unsigned int) {}
    void println(long) {}
    void println(unsigned long) {}
    void println(double) {}
    void println() {}
    void printf(const char*, ...) {}
};

extern StubSerial Serial;

#endif