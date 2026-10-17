#define TACPAD_USB_HID 1
#define TACPAD_REFERENCE_BOARD 1
#include <climits>
#include "../src/main.cpp"

void tick(uint32_t elapsed) { testMillis += elapsed; loop(); }
void press(uint8_t pin) { testPins[pin]=LOW; tick(1); tick(DebounceMs); }
void release(uint8_t pin) { testPins[pin]=HIGH; tick(1); tick(DebounceMs); }
void drain() { for (int i=0; i<100; ++i) tick(4); }

int main() {
    for (int& pin : testPins) pin=HIGH;
    setup();
    assert(USB.started && USB.product=="Tacpad 9" && USB.manufacturer=="Primadon");
    assert(Wire.sda==17 && Wire.scl==18 && displayReady);
    press(KeyPins[0]); release(KeyPins[0]);
    assert(TacpadHid::keyboard.reports.empty());
    testUsbReady=true; tick(1);
    const char expected[9]={'c','v','b','b','z','y',0,0,'`'};
    for (int i=0; i<9; ++i) {
        press(KeyPins[i]);
        if (i==6 || i==7) {
            assert(TacpadHid::consumer.held==(i==6?0xCD:0xE2));
        } else {
            const auto& held=TacpadHid::keyboard.held;
            assert(held.front()==KEY_LEFT_CTRL && held.back()==expected[i]);
            assert(held.size()==size_t(i==3?4:i==2?3:2));
        }
        tick(100); // A held physical button must not repeat the macro.
        assert(!TacpadHid::active);
        release(KeyPins[i]);
        drain();
    }
    const size_t before=TacpadHid::consumer.reports.size();
    encoderSteps=2; tick(1); drain();
    encoderSteps=-1; tick(1); drain();
    std::vector<uint16_t> nonzero;
    for (size_t i=before; i<TacpadHid::consumer.reports.size(); ++i)
        if (TacpadHid::consumer.reports[i]) nonzero.push_back(TacpadHid::consumer.reports[i]);
    assert((nonzero==std::vector<uint16_t>{0xE9,0xE9,0xEA}));
    TacpadHid::macro(0); tick(1);
    assert(TacpadHid::active);
    testUsbReady=false; tick(1);
    assert(TacpadHid::count==0 && !TacpadHid::active && TacpadHid::keyboard.held.empty());
    TacpadHid::volume(INT_MIN); // Bounded work even while disconnected.
    assert(TacpadHid::count==0);
    testUsbReady=true; tick(1); drain();
    const auto reportCount=TacpadHid::keyboard.reports.size();
    press(EncoderSW); tick(HoldMs); release(EncoderSW);
    assert(gameMode && TacpadHid::count==0);
    const auto afterMode=TacpadHid::keyboard.reports.size();
    assert(afterMode>=reportCount);
    press(KeyPins[0]); release(KeyPins[0]); drain();
    assert(TacpadHid::keyboard.reports.size()==afterMode);
    assert(board[0]==TicTacToe::Player);
    gameMode=false;
    TacpadHid::volume(INT_MAX);
    assert(TacpadHid::count==TacpadHid::Capacity);
    TacpadHid::cancel();
    testMillis=UINT32_MAX-10;
    TacpadHid::macro(0); tick(1); tick(21);
    assert(!TacpadHid::active && TacpadHid::keyboard.held.empty());
    std::puts("PASS: actual S3 firmware emits keyboard/media reports, bounded volume, releases on disconnect and mode change, no HID in XO, timer rollover.");
}
