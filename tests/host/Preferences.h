#pragma once
#include <cstdint>
#include <cstring>

// The stored value outlives end()/begin(), modeling a reboot of the application.
bool testNvsOpenOk = true, testNvsWriteOk = true, testNvsHasValue = false;
uint8_t testNvsValue = 0;
unsigned testNvsWrites = 0;
struct Preferences {
    bool opened = false;
    bool begin(const char* name, bool readOnly = false) {
        assert(std::strcmp(name, "tacpad9") == 0 && !readOnly);
        opened = testNvsOpenOk;
        return opened;
    }
    void end() { opened = false; }
    uint8_t getUChar(const char* key, uint8_t fallback = 0) {
        assert(opened && std::strcmp(key, "profile") == 0);
        return testNvsHasValue ? testNvsValue : fallback;
    }
    size_t putUChar(const char* key, uint8_t value) {
        assert(opened && std::strcmp(key, "profile") == 0);
        ++testNvsWrites;
        if (!testNvsWriteOk) return 0;
        testNvsHasValue = true;
        testNvsValue = value;
        return 1;
    }
};
