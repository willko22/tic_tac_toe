#pragma once
#include <string>
#include <vector>
#include <expected>

using namespace std;

class Game {
public:
    Game();
    ~Game();
    
    void run();
    
private:
    int _board_size = 3; // Size of the Tic Tac Toe board (can be changed)
    int _win_count = 3; // Number of symbols in a row to win
    char _symbols[3] = {' ', 'X', 'O'}; // Default player symbols
    bool _players_turn = true; // Track whose turn it is
    vector<int> _board; // Dynamic board that resizes based on _board_size

    string _board_sep;
    const string _move_sep = string(20, '=');

    void initialize();
    expected<int, string> player_move();
    void render();
    bool checkBoard(int index);
    bool checkWin(int index, int* count, int cur_ident);
    bool checkDiagonal(int start_row, int start_col, int diag_lenght, int cur_ident, int dir);

    void cleanup();
    
    // Private helper functions
    void setBoardSep();
    expected<void, string> setSymbols(int player_choice);
    void setBoardSize();
    void swapPlayers();
    
    bool _running;
};
