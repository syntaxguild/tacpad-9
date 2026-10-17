#pragma once
// Minimal host doubles for testing the actual firmware without an ESP32.
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

constexpr int HIGH = 1, LOW = 0, INPUT = 0, INPUT_PULLUP = 2, CHANGE = 3;
#define IRAM_ATTR
using portMUX_TYPE = int;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(mux) ((void)(mux))
#define portEXIT_CRITICAL(mux) ((void)(mux))
#define portENTER_CRITICAL_ISR(mux) ((void)(mux))
#define portEXIT_CRITICAL_ISR(mux) ((void)(mux))
uint32_t testMillis = 0;
int testPins[40];
uint32_t millis() { return testMillis; }
void pinMode(uint8_t, int) {}
int digitalRead(uint8_t pin) { return testPins[pin]; }
int digitalPinToInterrupt(uint8_t pin) { return pin; }
void attachInterrupt(int, void (*)(), int) {}
int constrain(int value, int low, int high) { return std::max(low, std::min(value, high)); }
struct SerialDouble {
    std::string output;
    void begin(int baud) { assert(baud == 115200); }
    void println(const char* text) { output += std::string(text) + "\n"; }
    template<typename... Args> void printf(const char* format, Args... args) {
        char line[256];
        std::snprintf(line, sizeof(line), format, args...);
        output += line;
    }
} Serial;
