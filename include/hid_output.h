#pragma once
#include <Arduino.h>
#include "board_config.h"

#if TACPAD_USB_HID
#include <USB.h>
#include <USBHID.h>
#include <USBHIDKeyboard.h>
#include <USBHIDConsumerControl.h>
#endif

namespace TacpadHid {
#if TACPAD_USB_HID
USBHIDKeyboard keyboard;
USBHIDConsumerControl consumer;
USBHID connection;
constexpr uint8_t Capacity = 16, VolumeUp = 9, VolumeDown = 10;
constexpr uint32_t HoldMs = 20;
uint8_t queue[Capacity], head = 0, tail = 0, count = 0;
bool active = false, activeConsumer = false, wasReady = false, resting = false;
uint32_t startedAt = 0, releasedAt = 0, dropped = 0;

inline void cancel() {
    head = tail = count = 0;
    keyboard.releaseAll();
    consumer.release();
    active = false;
    resting = false;
}

inline void begin() {
    USB.productName("Tacpad 9");
    USB.manufacturerName("Primadon");
    keyboard.begin();
    consumer.begin();
    USB.begin();
}

inline bool enqueue(uint8_t action) {
    if (action > VolumeDown || !connection.ready()) return false;
    if (count == Capacity) { ++dropped; return false; }
    queue[tail] = action;
    tail = (tail + 1) % Capacity;
    ++count;
    return true;
}

inline void macro(uint8_t index) {
    if (index < 9) enqueue(index);
}

inline void volume(int detents) {
    // One relative consumer report per detent. The OS chooses its volume step;
    // USB HID provides no host-volume percentage feedback to this device.
    const uint8_t action = detents > 0 ? VolumeUp : VolumeDown;
    unsigned remaining = detents < 0 ? unsigned(-(detents + 1)) + 1 : unsigned(detents);
    const unsigned available = unsigned(Capacity - count);
    if (remaining > available) remaining = available;
    while (remaining--) enqueue(action);
}

inline void update(uint32_t now) {
    if (!connection.ready()) {
        if (wasReady || active || count) cancel();
        wasReady = false;
        return;
    }
    if (!wasReady) {
        cancel();
        wasReady = true;
        return;
    }
    if (active) {
        if (uint32_t(now - startedAt) < HoldMs) return;
        if (activeConsumer) consumer.release();
        else keyboard.releaseAll();
        active = false;
        resting = true;
        releasedAt = now;
        return;  // Keep a released report between consecutive presses.
    }
    if (resting && uint32_t(now - releasedAt) < 4) return;
    resting = false;
    if (!count) return;
    const uint8_t action = queue[head];
    head = (head + 1) % Capacity;
    --count;
    activeConsumer = action == 6 || action == 7 || action >= VolumeUp;
    if (activeConsumer) {
        const uint16_t usage = action == 6 ? CONSUMER_CONTROL_PLAY_PAUSE :
            action == 7 ? CONSUMER_CONTROL_MUTE :
            action == VolumeUp ? CONSUMER_CONTROL_VOLUME_INCREMENT : CONSUMER_CONTROL_VOLUME_DECREMENT;
        consumer.press(usage);
    } else {
        keyboard.press(KEY_LEFT_CTRL);
        if (action == 2 || action == 3) keyboard.press(KEY_LEFT_SHIFT);
        if (action == 3) keyboard.press(KEY_LEFT_ALT);
        const uint8_t key = action == 0 ? 'c' : action == 1 ? 'v' :
            action == 2 || action == 3 ? 'b' : action == 4 ? 'z' : action == 5 ? 'y' : '`';
        keyboard.press(key);
    }
    active = true;
    startedAt = now;
}
#else
inline void begin() {}
inline void cancel() {}
inline void macro(uint8_t) {}
inline void volume(int) {}
inline void update(uint32_t) {}
#endif
}
