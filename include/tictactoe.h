#pragma once

// Pure C++ game rules, shared by the firmware and the exhaustive host check.
namespace TicTacToe {
constexpr char Empty = ' ';
constexpr char Player = 'X';
constexpr char AI = 'O';
constexpr char Draw = 'D';
constexpr int Lines[8][3] = {
    {0, 1, 2}, {3, 4, 5}, {6, 7, 8},
    {0, 3, 6}, {1, 4, 7}, {2, 5, 8},
    {0, 4, 8}, {2, 4, 6}
};
constexpr int MoveOrder[9] = {4, 0, 2, 6, 8, 1, 3, 5, 7};

inline char outcome(const char* board) {
    for (const auto& line : Lines) {
        const char cell = board[line[0]];
        if (cell != Empty && cell == board[line[1]] && cell == board[line[2]])
            return cell;
    }
    for (int i = 0; i < 9; ++i)
        if (board[i] == Empty) return Empty;
    return Draw;
}

inline int minimax(char* board, bool aiTurn, int depth, int alpha, int beta) {
    const char result = outcome(board);
    if (result == AI) return 10 - depth;
    if (result == Player) return depth - 10;
    if (result == Draw) return 0;

    int best = aiTurn ? -100 : 100;
    for (int cell : MoveOrder) {
        if (board[cell] != Empty) continue;
        board[cell] = aiTurn ? AI : Player;
        const int score = minimax(board, !aiTurn, depth + 1, alpha, beta);
        board[cell] = Empty;
        if (aiTurn) {
            if (score > best) best = score;
            if (best > alpha) alpha = best;
        } else {
            if (score < best) best = score;
            if (best < beta) beta = best;
        }
        if (beta <= alpha) break;
    }
    return best;
}

inline int bestMove(char* board) {
    if (outcome(board) != Empty) return -1;
    int move = -1;
    int best = -100;
    for (int cell : MoveOrder) {
        if (board[cell] != Empty) continue;
        board[cell] = AI;
        const int score = minimax(board, false, 1, -100, 100);
        board[cell] = Empty;
        if (score > best) {
            best = score;
            move = cell;
        }
    }
    return move;
}
}  // namespace TicTacToe
