// Primadon | TacPad-9 -- ESP32 / Arduino / PlatformIO
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#ifdef TACPAD_ILI9341
#include <Adafruit_ILI9341.h>
#else
#include <Adafruit_ST7789.h>
#endif
#include "tictactoe.h"

namespace {
constexpr uint8_t KeyPins[9] = {13, 12, 14, 27, 26, 25, 33, 15, 16};
constexpr uint8_t EncoderDT = 34, EncoderCLK = 35, EncoderSW = 32;
constexpr uint8_t TftMOSI = 23, TftSCLK = 18, TftCS = 5, TftDC = 2, TftRST = 4;
constexpr uint32_t DebounceMs = 20, HoldMs = 1500, HighlightMs = 140;
constexpr uint32_t TransitionMs = 320, ResultFrameMs = 280;
constexpr uint16_t Background = 0x0823, Surface = 0x1086, Cyan = 0x07FF;
constexpr uint16_t Magenta = 0xF81F, Red = 0xF986, White = 0xFFFF, Muted = 0x7C51;
constexpr int BoardX = 39, BoardY = 45, CellSize = 54;
const char* const Labels[9] = {
    "COPY", "PASTE", "BUILD", "BLENDER", "UNDO", "REDO", "PLAY", "MUTE", "TERMINAL"
};
const char* const Shortcuts[9] = {
    "CTRL+C", "CTRL+V", "CTRL+SHIFT+B", "LAUNCH BLENDER", "CTRL+Z",
    "CTRL+Y", "MEDIA PLAY/PAUSE", "MEDIA MUTE", "CTRL+`"
};

#ifdef TACPAD_ILI9341
Adafruit_ILI9341 tft(TftCS, TftDC, TftRST);
#else
Adafruit_ST7789 tft(TftCS, TftDC, TftRST);
#endif

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

    // Returns a single event per stable press; holding a key never repeats.
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
bool gameMode = false;
bool holdConsumed = false;
bool transitioning = false;
bool keysArmed = true;
bool resultBright = true;
bool highlighted[9] = {};
uint32_t highlightAt[9] = {};
uint32_t holdStartedAt = 0, transitionAt = 0, resultFrameAt = 0;
int transitionLine = -1;
int volume = 50;
char board[9];
char gameResult = TicTacToe::Empty;

// Both encoder signals use external pull-ups: GPIO34/35 have none internally.
// Interrupts capture rotation even during SPI drawing or the Minimax search.
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
        encoderQuarterSteps = 0;  // Reject an invalid two-bit jump.
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

void textAt(int x, int y, const char* text, uint16_t color, uint8_t size = 1) {
    tft.setTextSize(size);
    tft.setTextColor(color);
    tft.setCursor(x, y);
    tft.print(text);
}

void centered(int y, const char* text, uint16_t color, uint8_t size = 1) {
    textAt((240 - int(strlen(text)) * 6 * size) / 2, y, text, color, size);
}

void drawHeader(const char* subtitle) {
    tft.fillRect(0, 0, 240, 23, Surface);
    textAt(9, 8, "PRIMADON | TacPad-9", Cyan);
    tft.fillRect(215, 7, 15, 3, Magenta);
    tft.drawFastHLine(0, 22, 240, Cyan);
    centered(30, subtitle, White);
}

void drawMacroKey(int index) {
    const bool pressed = highlighted[index];
    const int x = 5 + (index % 3) * 78;
    const int y = 48 + (index / 3) * 44;
    tft.fillRect(x, y, 74, 41, Background);
    tft.fillRoundRect(x, y + (pressed ? 3 : 0), 73, 38, 4,
                      pressed ? Cyan : Surface);
    tft.drawRoundRect(x, y + (pressed ? 3 : 0), 73, 38, 4,
                      pressed ? White : Muted);
    char number[2] = {char('1' + index), '\0'};
    textAt(x + 5, y + 5 + (pressed ? 3 : 0), number, pressed ? Background : Magenta);
    textAt(x + (73 - int(strlen(Labels[index])) * 6) / 2,
           y + 22 + (pressed ? 3 : 0), Labels[index], pressed ? Background : White);
}

void drawVolume() {
    tft.fillRect(5, 185, 230, 31, Background);
    char bar[11];
    for (int i = 0; i < 10; ++i) bar[i] = i < volume / 10 ? '=' : ' ';
    bar[10] = '\0';
    char line[32];
    snprintf(line, sizeof(line), "VOLUME: [%s] %d%%", bar, volume);
    textAt(8, 188, line, Cyan);
    tft.drawRect(8, 202, 224, 8, Muted);
    tft.fillRect(10, 204, 220 * volume / 100, 4, Magenta);
}

void drawGameCell(int index) {
    const int x = BoardX + (index % 3) * CellSize;
    const int y = BoardY + (index / 3) * CellSize;
    tft.fillRect(x + 3, y + 3, CellSize - 5, CellSize - 5, Background);
    if (board[index] == TicTacToe::Player) {
        for (int thickness = -1; thickness <= 1; ++thickness) {
            tft.drawLine(x + 14, y + 14 + thickness, x + 40, y + 40 + thickness, Cyan);
            tft.drawLine(x + 40, y + 14 + thickness, x + 14, y + 40 + thickness, Cyan);
        }
    } else if (board[index] == TicTacToe::AI) {
        for (int radius = 14; radius <= 16; ++radius)
            tft.drawCircle(x + 27, y + 27, radius, Red);
    } else {
        char number[2] = {char('1' + index), '\0'};
        textAt(x + 24, y + 24, number, Muted);
    }
}

void drawGameBoard() {
    for (int i = 0; i < 9; ++i) drawGameCell(i);
    for (int i = 1; i <= 2; ++i) {
        tft.fillRect(BoardX + i * CellSize - 1, BoardY, 2, CellSize * 3, Cyan);
        tft.fillRect(BoardX, BoardY + i * CellSize - 1, CellSize * 3, 2, Cyan);
    }
    tft.drawRect(BoardX - 2, BoardY - 2, 166, 166, Magenta);
    tft.fillRect(0, 212, 240, 10, Background);
    centered(214, "YOU: X   /   AI: O", Cyan);
}

void drawResult() {
    const char* message = gameResult == TicTacToe::Player ? "PLAYER WINS!" :
                          gameResult == TicTacToe::AI ? "AI WINS!" : "DRAW GAME!";
    const uint16_t color = gameResult == TicTacToe::AI ? Red : Cyan;
    tft.fillRoundRect(12, 89, 216, 73, 5, Surface);
    tft.drawRoundRect(12, 89, 216, 73, 5, resultBright ? color : Magenta);
    centered(107, message, resultBright ? White : color, 2);
    centered(140, "PRESS ANY KEY TO RESTART", Muted);
}

void drawScreen() {
    tft.fillRect(0, 0, 240, 240, Background);
    drawHeader(gameMode ? "TIC-TAC-TOE VS AI" : "MACRO PAD / READY");
    if (gameMode) {
        drawGameBoard();
        if (gameResult != TicTacToe::Empty) drawResult();
    } else {
        for (int i = 0; i < 9; ++i) drawMacroKey(i);
        drawVolume();
    }
    centered(230, "HOLD KNOB 1.5s : SWITCH MODE", Muted);
}

void resetGame() {
    for (char& cell : board) cell = TicTacToe::Empty;
    gameResult = TicTacToe::Empty;
}

void switchMode(uint32_t now) {
    gameMode = !gameMode;
    resetGame();
    for (bool& active : highlighted) active = false;
    keysArmed = false;  // Keys held across a mode change cannot place a move.
    transitioning = true;
    transitionAt = now;
    transitionLine = -1;
    drawScreen();
    Serial.println(gameMode ? "MODE: TIC-TAC-TOE VS AI" : "MODE: MACRO PAD");
}

void handleGameKey(int index, uint32_t now) {
    if (gameResult != TicTacToe::Empty) {
        resetGame();
        drawScreen();
        keysArmed = false;  // The restart press never also places an X.
        Serial.println("GAME: NEW ROUND - PLAYER X STARTS");
        return;
    }
    if (board[index] != TicTacToe::Empty) return;
    board[index] = TicTacToe::Player;
    drawGameCell(index);
    Serial.printf("PLAYER X: %d\n", index + 1);
    gameResult = TicTacToe::outcome(board);
    if (gameResult == TicTacToe::Empty) {
        // Bounded nine-cell search, alpha-beta pruning, no artificial delay.
        const int move = TicTacToe::bestMove(board);
        if (move >= 0) {
            board[move] = TicTacToe::AI;
            drawGameCell(move);
            Serial.printf("AI O: %d\n", move + 1);
        }
        gameResult = TicTacToe::outcome(board);
    }
    if (gameResult != TicTacToe::Empty) {
        keysArmed = false;
        resultBright = true;
        resultFrameAt = now;
        drawResult();
        Serial.println(gameResult == TicTacToe::Player ? "PLAYER WINS!" :
                       gameResult == TicTacToe::AI ? "AI WINS!" : "DRAW GAME!");
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
    SPI.begin(TftSCLK, -1, TftMOSI, TftCS);
#ifdef TACPAD_ILI9341
    tft.begin(40000000);
    tft.setRotation(0);
    tft.fillScreen(Background);  // ILI9341 has an unused extra 80 pixels below UI.
#else
    tft.init(240, 240, SPI_MODE0);
    tft.setRotation(0);
#endif
    tft.setTextWrap(false);
    resetGame();
    drawScreen();
    Serial.println("PRIMADON | TacPad-9 | READY | MACRO PAD");
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
            drawVolume();
            Serial.printf("VOLUME: %d%%\n", volume);
        }
    }

    if (transitioning) {
        const uint32_t elapsed = now - transitionAt;
        if (elapsed >= TransitionMs) {
            transitioning = false;
            tft.drawRect(0, 0, 240, 240, Background);
        } else {
            const int frame = elapsed / 40;
            if (frame != transitionLine) {
                transitionLine = frame;
                tft.drawRect(0, 0, 240, 240, (frame & 1) ? Cyan : Magenta);
            }
        }
        return;
    }
    if (!keysArmed && allReleased) keysArmed = true;
    if (keysArmed) {
        for (int i = 0; i < 9; ++i) {
            if (!pressed[i]) continue;
            if (gameMode) {
                handleGameKey(i, now);
                break;  // At most one human turn per input scan.
            }
            highlighted[i] = true;
            highlightAt[i] = now;
            drawMacroKey(i);
            Serial.printf("MACRO %d [%s]: %s\n", i + 1, Labels[i], Shortcuts[i]);
        }
    }
    if (!gameMode) {
        for (int i = 0; i < 9; ++i) {
            if (highlighted[i] && !keys[i].down &&
                uint32_t(now - highlightAt[i]) >= HighlightMs) {
                highlighted[i] = false;
                drawMacroKey(i);
            }
        }
    } else if (gameResult != TicTacToe::Empty &&
               uint32_t(now - resultFrameAt) >= ResultFrameMs) {
        resultFrameAt = now;
        resultBright = !resultBright;
        drawResult();
    }
}
