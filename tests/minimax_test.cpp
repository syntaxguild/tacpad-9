// Build with a host C++11 compiler; see README.md. No Arduino dependencies.
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include "tictactoe.h"

using namespace TicTacToe;
unsigned playerWins = 0, aiWins = 0, draws = 0, aiPositions = 0;

// Independent full-tree oracle (no alpha-beta pruning or depth scoring).
int oracle(char* board, bool aiTurn) {
    const char result = outcome(board);
    if (result == AI) return 1;
    if (result == Player) return -1;
    if (result == Draw) return 0;
    int best = aiTurn ? -2 : 2;
    for (int i = 0; i < 9; ++i) {
        if (board[i] != Empty) continue;
        board[i] = aiTurn ? AI : Player;
        const int score = oracle(board, !aiTurn);
        board[i] = Empty;
        if (aiTurn ? score > best : score < best) best = score;
    }
    return best;
}

// Visit EVERY legal position, including positions requiring a block or a win.
bool visited[19683] = {};
void verifyAllPositions(char* board, bool aiTurn) {
    int code = 0;
    for (int i = 0; i < 9; ++i)
        code = code * 3 + (board[i] == Empty ? 0 : board[i] == Player ? 1 : 2);
    if (visited[code]) return;
    visited[code] = true;
    if (outcome(board) != Empty) {
        assert(bestMove(board) == -1);
        return;
    }
    if (aiTurn) {
        ++aiPositions;
        char before[9];
        memcpy(before, board, sizeof(before));
        const int expected = oracle(board, true);
        const int move = bestMove(board);
        assert(memcmp(before, board, sizeof(before)) == 0);
        assert(move >= 0 && move < 9 && board[move] == Empty);
        board[move] = AI;
        assert(oracle(board, false) == expected);
        board[move] = Empty;
    }
    for (int i = 0; i < 9; ++i) {
        if (board[i] != Empty) continue;
        board[i] = aiTurn ? AI : Player;
        verifyAllPositions(board, !aiTurn);
        board[i] = Empty;
    }
}

// Explore every possible human move sequence against the actual AI policy.
void playEveryHumanSequence(char* board, bool aiTurn) {
    const char result = outcome(board);
    if (result == Player) { ++playerWins; return; }
    if (result == AI) { ++aiWins; return; }
    if (result == Draw) { ++draws; return; }
    if (aiTurn) {
        const int move = bestMove(board);
        assert(move >= 0 && move < 9 && board[move] == Empty);
        board[move] = AI;
        playEveryHumanSequence(board, false);
        board[move] = Empty;
    } else {
        for (int i = 0; i < 9; ++i) {
            if (board[i] != Empty) continue;
            board[i] = Player;
            playEveryHumanSequence(board, true);
            board[i] = Empty;
        }
    }
}

int main() {
    // All eight winning lines for both symbols.
    char board[9];
    for (char symbol : {Player, AI}) {
        for (const auto& line : Lines) {
            memset(board, Empty, sizeof(board));
            for (int cell : line) board[cell] = symbol;
            assert(outcome(board) == symbol);
        }
    }
    const char drawn[9] = {Player, AI, Player, Player, AI, AI, AI, Player, Player};
    assert(outcome(drawn) == Draw);
    memset(board, Empty, sizeof(board));
    assert(outcome(board) == Empty);
    verifyAllPositions(board, false);
    playEveryHumanSequence(board, false);
    assert(playerWins == 0 && aiWins > 0 && draws > 0);
    for (char cell : board) assert(cell == Empty);
    std::printf("PASS: %u legal AI positions match full-tree oracle.\n", aiPositions);
    std::printf("PASS: %u human sequences: X wins=%u, O wins=%u, draws=%u.\n",
                playerWins + aiWins + draws, playerWins, aiWins, draws);
    return 0;
}
