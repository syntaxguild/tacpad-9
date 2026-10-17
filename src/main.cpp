// Primadon | TacPad-9 -- ESP32 / Arduino / PlatformIO / SSD1306 I2C
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "tictactoe.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

namespace {
constexpr uint8_t KeyPins[9] = {13, 12, 14, 27, 26, 25, 33, 15, 16};
constexpr uint8_t EncoderDT = 34, EncoderCLK = 35, EncoderSW = 32;
constexpr int BoardX = 0, BoardY = 8, CellSize = 8;
constexpr uint32_t DebounceMs = 20, HoldMs = 1500, HighlightMs = 140;
constexpr uint32_t TransitionMs = 240, ResultFrameMs = 300, FrameMs = 33;
const char* const Labels[9] = {
    "COPY", "PASTE", "BUILD", "BLENDER", "UNDO", "REDO", "PLAY", "MUTE", "TERMINAL"
};
const char* const Shortcuts[9] = {
    "CTRL+C", "CTRL+V", "CTRL+SHIFT+B", "LAUNCH BLENDER", "CTRL+Z",
    "CTRL+Y", "MEDIA PLAY/PAUSE", "MEDIA MUTE", "CTRL+`"
};

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1, 400000, 400000);

struct Button {
    uint8_t pin = 0;
    bool raw = false;
    bool down = false;
    uint32_t changedAt = 0;

    void begin(uint8_t gpio) {
        pin = gpio;
        pinMode(pin, INPUT_PULLUP);
        raw = down = digitalRead(pin) == LOW;
        changedAt = millis();
    }

    // One event per debounced press, even if the key remains held.
    bool update(uint32_t now) {
        const bool sample = digitalRead(pin) == LOW;
        if (sample != raw) {
            raw = sample;
            changedAt = now;
        }
        if (sample != down && uint32_t(now - changedAt) >= DebounceMs) {
            down = sample;
            return down;
        }
        return false;
    }
};

Button keys[9], modeButton;
bool gameMode = false, holdConsumed = false, keysArmed = true;
bool displayReady = false, screenDirty = true;
bool transitioning = false, resultBright = true, macroFlash = false;
uint32_t holdStartedAt = 0, transitionAt = 0, resultFrameAt = 0;
uint32_t macroPressedAt = 0, lastFrameAt = 0;
int transitionFrame = -1, lastMacro = -1, volume = 50;
char board[9];
char gameResult = TicTacToe::Empty;
// Session counters persist across rounds/mode changes; saturate at 65535.
uint16_t playerScore = 0, aiScore = 0, drawScore = 0;

// GPIO34/35 need external pull-ups. Capture every quadrature edge even while
// an OLED frame is being transferred over I2C or Minimax is searching.
portMUX_TYPE encoderMux = portMUX_INITIALIZER_UNLOCKED;
volatile int encoderSteps = 0;
volatile int encoderQuarterSteps = 0;
volatile uint8_t encoderState = 3;

void IRAM_ATTR readEncoder() {
    const uint8_t next = (digitalRead(EncoderCLK) << 1) | digitalRead(EncoderDT);
    portENTER_CRITICAL_ISR(&encoderMux);
    const uint8_t previous = encoderState;
    encoderState = next;
    if ((previous ^ next) == 3) {
        encoderQuarterSteps = 0;
    } else if (previous != next) {
        const bool clockwise = (previous == 3 && next == 1) ||
            (previous == 1 && next == 0) || (previous == 0 && next == 2) ||
            (previous == 2 && next == 3);
        encoderQuarterSteps += clockwise ? 1 : -1;
        if (next == 3) {
            if (encoderQuarterSteps == 4) ++encoderSteps;
            if (encoderQuarterSteps == -4) --encoderSteps;
            encoderQuarterSteps = 0;
        }
    }
    portEXIT_CRITICAL_ISR(&encoderMux);
}

void textAt(int x, int y, const char* text, uint16_t color = SSD1306_WHITE) {
    display.setTextColor(color);
    display.setCursor(x, y);
    display.print(text);
}

void drawMacro() {
    if (lastMacro < 0) {
        textAt(0, 12, "KEYS 1-9 READY");
    } else {
        char line[24];
        snprintf(line, sizeof(line), "%d:%s", lastMacro + 1, Labels[lastMacro]);
        if (macroFlash) display.fillRect(0, 11, 100, 10, SSD1306_WHITE);
        textAt(0, 12, line, macroFlash ? SSD1306_BLACK : SSD1306_WHITE);
    }
    char level[8];
    snprintf(level, sizeof(level), "%3d%%", volume);
    textAt(104, 12, level);
    display.drawRect(0, 24, SCREEN_WIDTH, 7, SSD1306_WHITE);
    display.fillRect(2, 26, (SCREEN_WIDTH - 4) * volume / 100, 3, SSD1306_WHITE);
}

void drawGame() {
    display.drawRect(BoardX, BoardY, CellSize * 3, CellSize * 3, SSD1306_WHITE);
    for (int i = 1; i <= 2; ++i) {
        display.drawFastVLine(BoardX + i * CellSize, BoardY, CellSize * 3, SSD1306_WHITE);
        display.drawFastHLine(BoardX, BoardY + i * CellSize, CellSize * 3, SSD1306_WHITE);
    }
    for (int i = 0; i < 9; ++i) {
        const int x = BoardX + (i % 3) * CellSize;
        const int y = BoardY + (i / 3) * CellSize;
        if (board[i] == TicTacToe::Player) {
            display.drawLine(x + 2, y + 2, x + 6, y + 6, SSD1306_WHITE);
            display.drawLine(x + 6, y + 2, x + 2, y + 6, SSD1306_WHITE);
        } else if (board[i] == TicTacToe::AI) {
            display.drawCircle(x + 4, y + 4, 2, SSD1306_WHITE);
        }
    }

    const bool finished = gameResult != TicTacToe::Empty;
    const char* status = !finished ? "YOUR TURN" :
        gameResult == TicTacToe::Player ? "PLAYER WINS!" :
        gameResult == TicTacToe::AI ? "AI WINS!" : "DRAW GAME!";
    if (finished && resultBright)
        display.fillRect(30, 8, 98, 8, SSD1306_WHITE);
    textAt(32, 8, status, finished && resultBright ? SSD1306_BLACK : SSD1306_WHITE);

    char score[24];
    snprintf(score, sizeof(score), "X:%u O:%u", unsigned(playerScore), unsigned(aiScore));
    textAt(32, 16, score);
    snprintf(score, sizeof(score), "D:%u %s", unsigned(drawScore), finished ? "NEW:ANY" : "KEY:1-9");
    textAt(32, 24, score);
}

void renderScreen() {
    if (!displayReady) return;
    display.clearDisplay();
    const bool flashHeader = transitioning && (transitionFrame & 1) != 0;
    if (flashHeader) display.fillRect(0, 0, SCREEN_WIDTH, 8, SSD1306_WHITE);
    const uint16_t headerColor = flashHeader ? SSD1306_BLACK : SSD1306_WHITE;
    textAt(0, 0, "PRIMADON", headerColor);
    textAt(98, 0, gameMode ? "XO AI" : "MACRO", headerColor);
    if (!gameMode) display.drawFastHLine(0, 9, SCREEN_WIDTH, SSD1306_WHITE);
    if (gameMode) drawGame();
    else drawMacro();
    display.display();
}

void resetGame() {
    for (char& cell : board) cell = TicTacToe::Empty;
    gameResult = TicTacToe::Empty;
    screenDirty = true;
}

void switchMode(uint32_t now) {
    gameMode = !gameMode;
    resetGame();
    macroFlash = false;
    keysArmed = false;  // Release all keys before accepting a new mode's input.
    transitioning = true;
    transitionAt = now;
    transitionFrame = 0;
    Serial.println(gameMode ? "MODE: TIC-TAC-TOE VS AI" : "MODE: MACRO PAD");
}

void handleGameKey(int index, uint32_t now) {
    if (gameResult != TicTacToe::Empty) {
        resetGame();
        keysArmed = false;  // A restart press cannot also place an X.
        Serial.println("GAME: NEW ROUND - PLAYER X STARTS");
        return;
    }
    if (board[index] != TicTacToe::Empty) return;
    board[index] = TicTacToe::Player;
    Serial.printf("PLAYER X: %d\n", index + 1);
    gameResult = TicTacToe::outcome(board);
    if (gameResult == TicTacToe::Empty) {
        const int move = TicTacToe::bestMove(board);
        if (move >= 0) {
            board[move] = TicTacToe::AI;
            Serial.printf("AI O: %d\n", move + 1);
        }
        gameResult = TicTacToe::outcome(board);
    }
    screenDirty = true;
    if (gameResult != TicTacToe::Empty) {
        keysArmed = false;
        resultBright = true;
        resultFrameAt = now;
        uint16_t& score = gameResult == TicTacToe::Player ? playerScore :
                          gameResult == TicTacToe::AI ? aiScore : drawScore;
        if (score < UINT16_MAX) ++score;
        Serial.println(gameResult == TicTacToe::Player ? "PLAYER WINS!" :
                       gameResult == TicTacToe::AI ? "AI WINS!" : "DRAW GAME!");
        Serial.printf("SCORE: YOU=%u AI=%u DRAW=%u\n",
                      unsigned(playerScore), unsigned(aiScore), unsigned(drawScore));
    }
}
}  // namespace

void setup() {
    Serial.begin(115200);
    for (int i = 0; i < 9; ++i) keys[i].begin(KeyPins[i]);
    modeButton.begin(EncoderSW);
    holdStartedAt = millis();
    pinMode(EncoderDT, INPUT);  // External 10k pull-up to 3V3.
    pinMode(EncoderCLK, INPUT);
    encoderState = (digitalRead(EncoderCLK) << 1) | digitalRead(EncoderDT);
    attachInterrupt(digitalPinToInterrupt(EncoderCLK), readEncoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(EncoderDT), readEncoder, CHANGE);

    Wire.begin(21, 22);
    Wire.setClock(400000);
    Wire.setTimeOut(50);
    // Always initialize the controller. A separate address-only probe must not
    // prevent the SSD1306 init sequence from ever reaching the display.
    displayReady = display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    if (!displayReady)
        Serial.println("OLED ERROR: framebuffer allocation failed. Serial controls remain active.");
    display.setTextSize(1);
    display.setTextWrap(false);
    resetGame();
    renderScreen();
    lastFrameAt = millis();
    screenDirty = false;
    Serial.println("PRIMADON | TacPad-9 | READY | MACRO PAD | OLED 128x32");
    Serial.println("Hold encoder SW for 1500ms to switch modes.");
    Serial.println("VOLUME: 50%");
}

void loop() {
    const uint32_t now = millis();
    bool pressed[9];
    bool allReleased = true;
    for (int i = 0; i < 9; ++i) {
        pressed[i] = keys[i].update(now);
        if (keys[i].raw || keys[i].down) allReleased = false;
    }
    const bool modePressed = modeButton.update(now);
    if (modePressed) holdStartedAt = now;
    if (!modeButton.down) holdConsumed = false;
    if (modeButton.down && modeButton.raw && !holdConsumed &&
        uint32_t(now - holdStartedAt) >= HoldMs) {
        holdConsumed = true;
        switchMode(now);
    }

    int steps;
    portENTER_CRITICAL(&encoderMux);
    steps = encoderSteps;
    encoderSteps = 0;
    portEXIT_CRITICAL(&encoderMux);
    if (!gameMode && steps != 0) {
        const int next = constrain(volume + steps * 5, 0, 100);
        if (next != volume) {
            volume = next;
            screenDirty = true;
            Serial.printf("VOLUME: %d%%\n", volume);
        }
    }

    if (!keysArmed && allReleased) keysArmed = true;
    if (keysArmed) {
        for (int i = 0; i < 9; ++i) {
            if (!pressed[i]) continue;
            if (gameMode) {
                handleGameKey(i, now);
                break;
            }
            lastMacro = i;
            macroFlash = true;
            macroPressedAt = now;
            screenDirty = true;
            Serial.printf("MACRO %d [%s]: %s\n", i + 1, Labels[i], Shortcuts[i]);
        }
    }
    if (!gameMode && macroFlash && !keys[lastMacro].down &&
        uint32_t(now - macroPressedAt) >= HighlightMs) {
        macroFlash = false;
        screenDirty = true;
    }
    if (gameMode && gameResult != TicTacToe::Empty &&
        uint32_t(now - resultFrameAt) >= ResultFrameMs) {
        resultFrameAt = now;
        resultBright = !resultBright;
        screenDirty = true;
    }
    if (transitioning) {
        const uint32_t elapsed = now - transitionAt;
        const int frame = elapsed / 40;
        if (elapsed >= TransitionMs) {
            transitioning = false;
            screenDirty = true;
        } else if (frame != transitionFrame) {
            transitionFrame = frame;
            screenDirty = true;
        }
    }
    // Send at most one buffered I2C frame per 33ms, and only when UI changes.
    if (screenDirty && uint32_t(now - lastFrameAt) >= FrameMs) {
        renderScreen();
        lastFrameAt = millis();
        screenDirty = false;
    }
}
