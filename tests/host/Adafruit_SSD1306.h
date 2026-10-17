#pragma once
#include "Wire.h"
constexpr uint16_t SSD1306_BLACK = 0, SSD1306_WHITE = 1;
constexpr uint8_t SSD1306_SWITCHCAPVCC = 2;
struct Adafruit_SSD1306 {
    int width, height, cursorX = 0, cursorY = 0, frames = 0, initCalls = 0;
    Adafruit_SSD1306(int w, int h, TwoWire*, int, uint32_t, uint32_t) : width(w), height(h) {}
    bool begin(uint8_t supply, uint8_t address) {
        assert(Wire.sda == 21 && Wire.scl == 22);
        assert(supply == SSD1306_SWITCHCAPVCC && address == 0x3C);
        ++initCalls;
        return true;
    }
    void bounds(int x, int y, int w, int h) {
        assert(x >= 0 && y >= 0 && w >= 0 && h >= 0);
        assert(x + w <= width && y + h <= height);
    }
    void setTextSize(int size) { assert(size == 1); }
    void setTextWrap(bool wrap) { assert(!wrap); }
    void setTextColor(uint16_t) {}
    void setCursor(int x, int y) { cursorX = x; cursorY = y; }
    void print(const char* text) { bounds(cursorX, cursorY, std::strlen(text) * 6, 8); }
    void clearDisplay() {}
    void display() { ++frames; }
    void fillRect(int x, int y, int w, int h, uint16_t) { bounds(x, y, w, h); }
    void drawRect(int x, int y, int w, int h, uint16_t) { bounds(x, y, w, h); }
    void drawFastHLine(int x, int y, int w, uint16_t) { bounds(x, y, w, 1); }
    void drawFastVLine(int x, int y, int h, uint16_t) { bounds(x, y, 1, h); }
    void drawLine(int x1, int y1, int x2, int y2, uint16_t) {
        bounds(x1, y1, 1, 1);
        bounds(x2, y2, 1, 1);
    }
    void drawCircle(int x, int y, int radius, uint16_t) {
        bounds(x - radius, y - radius, radius * 2 + 1, radius * 2 + 1);
    }
};
