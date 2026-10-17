#pragma once
#include <Arduino.h>
#include "board_config.h"
#include "keymap.h"

#if TACPAD_USB_HID
#include <USB.h>
#include <USBHID.h>
#include <USBHIDKeyboard.h>
#include <USBHIDConsumerControl.h>
#endif

namespace TacpadHid {
#if TACPAD_USB_HID
static_assert(TacpadKeymap::Tab == KEY_TAB && TacpadKeymap::F12 == KEY_F12, "USB key encoding changed");
USBHIDKeyboard keyboard;
USBHIDConsumerControl consumer;
USBHID connection;
constexpr uint8_t Capacity = 16;
constexpr uint32_t HoldMs = 20;
TacpadKeymap::Binding queue[Capacity];
uint8_t head = 0, tail = 0, count = 0;
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

inline bool enqueue(const TacpadKeymap::Binding& action) {
    if ((action.key == 0) == (action.consumer == 0) || !connection.ready()) return false;
    if (count == Capacity) { ++dropped; return false; }
    queue[tail] = action;
    tail = (tail + 1) % Capacity;
    ++count;
    return true;
}

inline void macro(uint8_t index, uint8_t profileId = TacpadKeymap::Coding) {
    if (index < TacpadKeymap::KeyCount && profileId < TacpadKeymap::Count)
        enqueue(TacpadKeymap::profile(profileId).keys[index]);
}

inline void volume(int detents) {
    // One relative consumer report per detent. The OS chooses its volume step;
    // USB HID provides no host-volume percentage feedback to this device.
    const auto& action = detents > 0 ? TacpadKeymap::VolumeUp : TacpadKeymap::VolumeDown;
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
    const auto action = queue[head];
    head = (head + 1) % Capacity;
    --count;
    activeConsumer = action.consumer != 0;
    if (activeConsumer) {
        consumer.press(action.consumer);
    } else {
        if (action.modifiers & TacpadKeymap::Ctrl) keyboard.press(KEY_LEFT_CTRL);
        if (action.modifiers & TacpadKeymap::Shift) keyboard.press(KEY_LEFT_SHIFT);
        if (action.modifiers & TacpadKeymap::Alt) keyboard.press(KEY_LEFT_ALT);
        keyboard.press(action.key);
    }
    active = true;
    startedAt = now;
}
#else
inline void begin() {}
inline void cancel() {}
inline void macro(uint8_t, uint8_t = TacpadKeymap::Coding) {}
inline void volume(int) {}
inline void update(uint32_t) {}
#endif
}
