#define TACPAD_USB_HID 1
#define TACPAD_REFERENCE_BOARD 1
#include "../src/main.cpp"

void tick(uint32_t elapsed) { testMillis += elapsed; loop(); }
void press(uint8_t pin) { testPins[pin] = LOW; tick(1); tick(DebounceMs); }
void release(uint8_t pin) { testPins[pin] = HIGH; tick(1); tick(DebounceMs); }
void tapEncoder() { press(EncoderSW); release(EncoderSW); }
void drain() { for (int i = 0; i < 100; ++i) tick(4); }
bool shown(const std::string& text) {
    return std::find(display.texts.begin(), display.texts.end(), text) != display.texts.end();
}

int main(int argc, char** argv) {
    if (argc == 2 && std::strcmp(argv[1], "--dump") == 0) {
        // The docs runner consumes these actual compiled bindings as TSV.
        for (const auto& profile : TacpadKeymap::Profiles) {
            for (int i = 0; i < 9; ++i) {
                char text[32];
                TacpadKeymap::shortcut(profile.keys[i], text, sizeof(text));
                std::printf("%s\t%d\t%s\t%s\n", profile.name, i + 1, profile.keys[i].label, text);
            }
        }
        return 0;
    }
    for (int& pin : testPins) pin = HIGH;
    setup();
    assert(activeProfile == TacpadKeymap::Coding && profileSaved && testNvsWrites == 0);
    testUsbReady = true;
    tick(1);

    // Independent expected reports catch an incorrect binding, not merely whether
    // the dispatcher agrees with its own table. Row order is physical key order.
    struct Expected { uint8_t modifiers, key; uint16_t usage; };
    const Expected expected[3][9] = {
        {{1,'c',0},{1,'v',0},{3,'b',0},{7,'b',0},{1,'z',0},{1,'y',0},{0,0,0xCD},{0,0,0xE2},{1,'`',0}},
        {{0,'g',0},{0,'r',0},{0,'s',0},{0,'e',0},{1,'z',0},{3,'z',0},{0,KEY_TAB,0},{4,'a',0},{0,KEY_F12,0}},
        {{0,0,0xB6},{0,0,0xCD},{0,0,0xB5},{0,0,0xEA},{0,0,0xE2},{0,0,0xE9},{0,0,0xB7},{0,0,0xB4},{0,0,0xB3}}
    };
    for (int profileId = 0; profileId < 3; ++profileId) {
        assert(activeProfile == profileId);
        renderScreen();
        assert(shown(TacpadKeymap::profile(profileId).name));
        for (int i = 0; i < 9; ++i) {
            const auto& want = expected[profileId][i];
            const auto& binding = TacpadKeymap::profile(profileId).keys[i];
            assert(binding.key == want.key && binding.modifiers == want.modifiers && binding.consumer == want.usage);
            assert(std::strlen(binding.label) <= 13);
            press(KeyPins[i]);
            if (want.usage) {
                assert(TacpadHid::consumer.held == want.usage && TacpadHid::keyboard.held.empty());
            } else {
                std::vector<uint8_t> keys;
                if (want.modifiers & 1) keys.push_back(KEY_LEFT_CTRL);
                if (want.modifiers & 2) keys.push_back(KEY_LEFT_SHIFT);
                if (want.modifiers & 4) keys.push_back(KEY_LEFT_ALT);
                keys.push_back(want.key);
                assert(TacpadHid::keyboard.held == keys && TacpadHid::consumer.held == 0);
            }
            renderScreen();
            assert(shown(std::to_string(i + 1) + ":" + binding.label));
            char shortcut[32];
            TacpadKeymap::shortcut(binding, shortcut, sizeof(shortcut));
            assert(Serial.output.find(std::string("[") + binding.label + "]: " + shortcut) != std::string::npos);
            release(KeyPins[i]); drain();
            assert(TacpadHid::keyboard.held.empty() && TacpadHid::consumer.held == 0);
        }
        // Selecting a profile clears a queued/held action and gates held keys.
        TacpadHid::macro(0, activeProfile);
        tick(1);
        assert(TacpadHid::active);
        tapEncoder();
        assert(activeProfile == (profileId + 1) % 3 && !TacpadHid::active && TacpadHid::count == 0);
        assert(testNvsWrites == unsigned(profileId + 1) && testNvsValue == activeProfile);
        activeProfile = 255;
        loadProfile();  // Reopen storage as setup does after restart.
        assert(activeProfile == (profileId + 1) % 3 && testNvsWrites == unsigned(profileId + 1));
    }

    // Bounce and rotation never persist a profile. A long press is only XO.
    const auto writes = testNvsWrites;
    testPins[EncoderSW] = LOW; tick(1); tick(5);
    testPins[EncoderSW] = HIGH; tick(1); tick(DebounceMs);
    encoderSteps = 3; tick(1); drain();
    assert(testNvsWrites == writes && activeProfile == TacpadKeymap::Coding);
    press(EncoderSW); tick(HoldMs); tick(HoldMs * 2); release(EncoderSW);
    assert(gameMode && activeProfile == TacpadKeymap::Coding && testNvsWrites == writes);
    tapEncoder();
    assert(gameMode && testNvsWrites == writes);
    press(EncoderSW); tick(HoldMs); release(EncoderSW);
    assert(!gameMode && activeProfile == TacpadKeymap::Coding && testNvsWrites == writes);

    // A key held during the selection cannot fire the next profile's action.
    press(KeyPins[0]);
    tapEncoder();
    assert(activeProfile == TacpadKeymap::Blender && !keysArmed);
    const auto reports = TacpadHid::keyboard.reports.size();
    tick(100);
    assert(TacpadHid::keyboard.reports.size() == reports);
    release(KeyPins[0]); tick(1);
    press(KeyPins[0]);
    assert((TacpadHid::keyboard.held == std::vector<uint8_t>{'g'}));
    release(KeyPins[0]); drain();

    testNvsWriteOk = false;
    const uint8_t stored = testNvsValue;
    tapEncoder();
    assert(activeProfile == TacpadKeymap::Media && !profileSaved && testNvsValue == stored);
    renderScreen(); assert(shown("MEDIA*"));
    loadProfile(); assert(activeProfile == stored && profileSaved);
    testNvsWriteOk = true;
    testNvsValue = 255;
    loadProfile(); assert(activeProfile == TacpadKeymap::Coding);
    testNvsOpenOk = false;
    loadProfile(); assert(activeProfile == TacpadKeymap::Coding && !profileSaved);
    const auto failedWrites = testNvsWrites;
    tapEncoder();
    assert(activeProfile == TacpadKeymap::Blender && !profileSaved && testNvsWrites == failedWrites);
    renderScreen(); assert(shown("BLENDER*"));
    testNvsOpenOk = true;
    loadProfile();

    // Startup with the button already down must not count its release as a tap.
    testPins[EncoderSW] = LOW; modeButton.begin(EncoderSW); encoderTapArmed = false;
    release(EncoderSW);
    assert(activeProfile == TacpadKeymap::Coding);
    testMillis = UINT32_MAX - 100;
    press(EncoderSW); tick(HoldMs); release(EncoderSW);
    assert(gameMode && activeProfile == TacpadKeymap::Coding);
    std::puts("PASS: 27 report mappings, shared OLED/Serial labels, profile cycling, persistence/failure fallback, bounce, held-key gating, XO isolation and timer rollover.");
}
