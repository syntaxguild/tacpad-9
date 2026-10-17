#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace TacpadKeymap {
enum Modifier : uint8_t { None = 0, Ctrl = 1, Shift = 2, Alt = 4 };
// Arduino Keyboard's encoded non-printing keys (checked against the USB header).
constexpr uint8_t Tab = 0xB3, F12 = 0xCD;
constexpr uint8_t KeyCount = 9;
// Stored IDs are stable: append new profiles rather than reordering these.
enum ProfileId : uint8_t { Coding, Blender, Media, Count };

struct Binding {
    const char* label;
    uint8_t modifiers;
    uint8_t key;
    uint16_t consumer;
};

struct Profile {
    const char* name;
    Binding keys[KeyCount];
};

constexpr Binding VolumeUp = {"VOL+", None, 0, 0xE9};
constexpr Binding VolumeDown = {"VOL-", None, 0, 0xEA};
constexpr Profile Profiles[] = {
    {"CODING", {
        {"COPY", Ctrl, 'c', 0},
        {"PASTE", Ctrl, 'v', 0},
        {"BUILD", Ctrl | Shift, 'b', 0},
        {"BLENDER", Ctrl | Alt | Shift, 'b', 0},
        {"UNDO", Ctrl, 'z', 0},
        {"REDO", Ctrl, 'y', 0},
        {"PLAY", None, 0, 0xCD},
        {"MUTE", None, 0, 0xE2},
        {"TERMINAL", Ctrl, '`', 0},
    }},
    {"BLENDER", {
        {"MOVE", None, 'g', 0},
        {"ROTATE", None, 'r', 0},
        {"SCALE", None, 's', 0},
        {"EXTRUDE", None, 'e', 0},
        {"UNDO", Ctrl, 'z', 0},
        {"REDO", Ctrl | Shift, 'z', 0},
        {"EDIT", None, Tab, 0},
        {"DESELECT", Alt, 'a', 0},
        {"RENDER", None, F12, 0},
    }},
    {"MEDIA", {
        {"PREVIOUS", None, 0, 0xB6},
        {"PLAY", None, 0, 0xCD},
        {"NEXT", None, 0, 0xB5},
        VolumeDown,
        {"MUTE", None, 0, 0xE2},
        VolumeUp,
        {"STOP", None, 0, 0xB7},
        {"REWIND", None, 0, 0xB4},
        {"FORWARD", None, 0, 0xB3},
    }},
};
static_assert(sizeof(Profiles) / sizeof(Profiles[0]) == Count, "Profile count mismatch");

inline const Profile& profile(uint8_t id) {
    return Profiles[id < Count ? id : uint8_t(Coding)];
}

// Derive diagnostic text from the very same fields that drive USB reports.
inline void shortcut(const Binding& binding, char* text, size_t size) {
    if (binding.consumer) {
        std::snprintf(text, size, "MEDIA %s", binding.label);
        return;
    }
    char printable[2] = {char(binding.key >= 'a' && binding.key <= 'z' ? binding.key - 32 : binding.key), 0};
    const char* name = binding.key == Tab ? "TAB" : binding.key == F12 ? "F12" : printable;
    std::snprintf(text, size, "%s%s%s%s", binding.modifiers & Ctrl ? "CTRL+" : "",
                  binding.modifiers & Alt ? "ALT+" : "", binding.modifiers & Shift ? "SHIFT+" : "", name);
}
}  // namespace TacpadKeymap
