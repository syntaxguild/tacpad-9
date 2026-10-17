#pragma once
#include <string>
struct USBControlDouble {
    std::string product, manufacturer;
    bool started = false;
    void productName(const char* s) { product = s; }
    void manufacturerName(const char* s) { manufacturer = s; }
    void begin() { started = true; }
} USB;
