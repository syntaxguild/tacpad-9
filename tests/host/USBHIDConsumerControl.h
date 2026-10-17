#pragma once
#include <vector>
constexpr uint16_t CONSUMER_CONTROL_PLAY_PAUSE=0xCD, CONSUMER_CONTROL_MUTE=0xE2;
constexpr uint16_t CONSUMER_CONTROL_VOLUME_INCREMENT=0xE9, CONSUMER_CONTROL_VOLUME_DECREMENT=0xEA;
struct USBHIDConsumerControl {
    uint16_t held = 0;
    std::vector<uint16_t> reports;
    void begin() {}
    void press(uint16_t usage) { held=usage; reports.push_back(usage); }
    void release() { held=0; reports.push_back(0); }
};
