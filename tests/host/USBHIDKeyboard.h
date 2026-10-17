#pragma once
#include <vector>
#include <algorithm>
constexpr uint8_t KEY_LEFT_CTRL=0x80, KEY_LEFT_SHIFT=0x81, KEY_LEFT_ALT=0x82;
struct USBHIDKeyboard {
    std::vector<uint8_t> held;
    std::vector<std::vector<uint8_t>> reports;
    void begin() {}
    void press(uint8_t key) {
        if (std::find(held.begin(), held.end(), key) == held.end()) held.push_back(key);
        reports.push_back(held);
    }
    void releaseAll() { held.clear(); reports.push_back(held); }
};
