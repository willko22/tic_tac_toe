#pragma once
#include <string>
#include <vector>
#include <expected>


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
    // vector<int> _board; // Dynamic board that resizes based on _board_size
    bool _with_ai = true; // Flag to indicate if AI is playing
    int _last_move = -1; // Track the last move made
    std::vector<bool> _player1;
    std::vector<bool> _player2;
    std::vector<bool> _board; // Board state


    std::string _board_sep;
    const std::string _move_sep = std::string(20, '=');

    void initialize();
    std::expected<int, std::string> playerMove(std::vector<bool>& player);
    void render();
    int checkBoard(int index, std::vector<bool>& player);
    int aiTurn()   ; // Placeholder for AI logic
    int minimax(int depth, bool maximizingPlayer, int last_move); // , int alpha, int beta later for alpha-beta pruning
    void placePiece(int index, std::vector<bool>& player, bool remove = false);
    int evaluatePosition(); // Evaluate the current board position for AI

    void cleanup();
    
    // Private helper functions
    void setBoardSep();
    std::expected<void, std::string> setSymbols(int player_choice);
    void setBoardSize();
    void swapPlayers();
    
    bool _running;
};
