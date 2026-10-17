// Uses host doubles to run the real firmware. See README for build command.
#include <initializer_list>
#include "../src/main.cpp"

void tick(uint32_t elapsed) {
    testMillis += elapsed;
    loop();
}

void press(uint8_t pin) {
    testPins[pin] = LOW;
    tick(1);
    tick(DebounceMs);
}

void release(uint8_t pin) {
    testPins[pin] = HIGH;
    tick(1);
    tick(DebounceMs);
}

int main() {
    for (int& pin : testPins) pin = HIGH;
    setup();
    assert(display.width == 128 && display.height == 32);
    assert(displayReady && display.initCalls == 1 && display.frames == 1);
    assert(Wire.probes == 0);  // A failed address probe must never gate init.
    assert(Serial.output.find("OLED ERROR") == std::string::npos);
    assert(Serial.output.find("OLED 128x32") != std::string::npos);

    for (int i = 0; i < 9; ++i) {
        press(KeyPins[i]);
        assert(lastMacro == i && macroFlash);
        renderScreen();
        release(KeyPins[i]);
        tick(HighlightMs);
        assert(!macroFlash);
    }
    encoderSteps = 20;
    tick(FrameMs);
    assert(volume == 100);
    encoderSteps = -40;
    tick(FrameMs);
    assert(volume == 0);

    press(EncoderSW);
    tick(HoldMs - 1);
    assert(!gameMode);
    tick(1);
    assert(gameMode);
    tick(HoldMs * 2);
    assert(gameMode);  // Holding cannot toggle a second time.
    release(EncoderSW);
    press(KeyPins[0]);
    assert(board[0] == TicTacToe::Player && board[4] == TicTacToe::AI);
    release(KeyPins[0]);

    // Exercise every symbol position, result, and widest possible score text.
    playerScore = aiScore = drawScore = UINT16_MAX;
    for (char symbol : {TicTacToe::Player, TicTacToe::AI, TicTacToe::Empty}) {
        for (char& cell : board) cell = symbol;
        for (char result : {TicTacToe::Empty, TicTacToe::Player, TicTacToe::AI, TicTacToe::Draw}) {
            gameResult = result;
            resultBright = false;
            renderScreen();
            resultBright = true;
            renderScreen();
        }
    }
    gameResult = TicTacToe::Draw;
    press(KeyPins[8]);
    assert(gameResult == TicTacToe::Empty && drawScore == UINT16_MAX);
    for (char cell : board) assert(cell == TicTacToe::Empty);
    release(KeyPins[8]);
    press(EncoderSW);
    tick(HoldMs);
    assert(!gameMode);
    release(EncoderSW);
    transitioning = true;
    transitionFrame = 1;
    renderScreen();
    std::puts("PASS: OLED init after Wire.begin(21,22), no probe gate, all UI inside 128x32.");
    std::puts("PASS: nine macros, volume bounds, 1.5s hold, immediate AI response, restart and scores.");
}
