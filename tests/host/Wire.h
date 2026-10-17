#pragma once
#include "Arduino.h"
struct TwoWire {
    int sda = -1, scl = -1, probes = 0;
    bool begin(int data, int clock) { sda = data; scl = clock; return true; }
    void setClock(uint32_t) {}
    void setTimeOut(uint16_t) {}
    void beginTransmission(uint8_t) { ++probes; }
    // Reproduce a failed pre-init address probe. Init must still be attempted.
    int endTransmission() { return 2; }
} Wire;
