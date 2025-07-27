#pragma once
#include <string>
#include <vector>
#include <expected>
#include <unordered_set>
#include <unordered_map>
#include <cstdint>

using BoardVec = std::vector<std::uint_fast8_t>;
using zobrist_t = uint64_t;


class Game
{
public:
    Game();
    ~Game();

    void run();

private:
    int _board_size = 3;                // Size of the Tic Tac Toe board (can be changed)
    int _win_count = 3;                 // Number of symbols in a row to win
    char _symbols[3] = {' ', 'X', 'O'}; // Default player symbols
    bool _players_turn = true;          // Track whose turn it is
    // vector<int> _board; // Dynamic board that resizes based on _board_size
    bool _with_ai = true; // Flag to indicate if AI is playing
    int _last_move = -1;  // Track the last move made
    BoardVec _board; // Board state, using uint_fast8_t for efficiency
    // std::vector<bool> _player1;
    // std::vector<bool> _player2;
    // std::vector<bool> _board; // Board state
    std::unordered_map<int, std::unordered_set<int>> _adjuscent_map; // map where keys are board index and values are sets of adjacent board indices
    std::string _board_sep;
    const std::string _move_sep = std::string(20, '=');

    uint_fast8_t _hash_treshold = 3; // Threshold for using transposition table


    std::unordered_map<zobrist_t, int> _trans_table;
    zobrist_t _zobrist_hash = 0;

    

    // Zobrist table: [cell index][player_id 1/2]
    std::vector<std::array<zobrist_t, 3>> _zobrist_table;


    void initialize();

    std::expected<int, std::string> playerMove(bool player1);
    void render();
    int checkBoard(const int index, const std::uint_fast8_t player) const;
    int aiTurn();                                                                      // Placeholder for AI logic
    int minimax(int depth, bool maximizingPlayer, int last_move, int alpha, int beta); // , int alpha, int beta later for alpha-beta pruning
    inline std::vector<int> getAvailableMoves(const bool player_turn, const int last_move) ;

    void generateAdjuscentMap();
    inline int evalBoard(const bool player1) const;

    inline void applyMove(int index, uint_fast8_t player);
    inline void undoMove(int index, uint_fast8_t prev_player);

    void cleanup();

    // Private helper functions
    void setBoardSep();
    std::expected<void, std::string> setSymbols(int player_choice);
    void setBoardSize();
    void swapPlayers();

    bool _running;
};
