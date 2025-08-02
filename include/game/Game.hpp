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
    //########## CONSTRUCTORS/DESTRUCTORS ##########
    Game();
    ~Game();
    
    //########## PUBLIC INTERFACE ##########
    void run();

private:
    //########## CONSTANTS ##########
    static constexpr int WIN_SCORE = 100000;
    static constexpr int LOSS_SCORE = -100000;
    static constexpr int TIE_SCORE = 0;
    static constexpr const char* MOVE_SEPARATOR = "===========================================================";
    
    //========== Game Configuration ==========
    int _board_size = 3;
    int _win_count = 3;
    char _symbols[3] = {' ', 'X', 'O'};
    bool _players_turn = true;
    bool _with_ai = true;
    int _last_move = -1;
    bool _running = false;
    
    //========== Board State ==========
    BoardVec _board;
    std::string _board_separator;
    
    //========== Move Generation Optimization ==========
    std::unordered_map<int, std::vector<int>> _adjacent_map;
    std::vector<int> _active_square_list;
    std::unordered_set<int> _active_square_set;
    std::vector<int> _neighbor_count;
    
    //========== AI Optimization ==========
    std::unordered_map<zobrist_t, int> _transposition_table;
    zobrist_t _zobrist_hash = 0;
    std::vector<std::array<zobrist_t, 3>> _zobrist_table;
    uint_fast8_t _hash_threshold = 3;

    //########## PRIVATE METHODS ##########
    //========== Core Game Methods ==========
    void initialize();
    void cleanup();
    void setBoardSize();
    void generateAdjacentMap();
    
    //========== Player Interaction ==========
    std::expected<int, std::string> playerMove(uint_fast8_t player_id);
    std::expected<void, std::string> setSymbols(int player_choice);
    void render() const;
    
    //========== Game Logic ==========
    int checkBoard(int index, uint_fast8_t player) const;
    
    //========== AI Methods ==========
    int aiTurn();
    int minimax(int depth, bool maximizing_player, int alpha, int beta);
    int evalBoard(bool maximizing_player) const;
    int evaluateMoveThreat(int move, uint_fast8_t player) const;
    
    //========== Board State Management ==========
    void markActive(int index);
    void unmarkActive(int index);
    void applyMove(int index, uint_fast8_t player);
    void undoMove(int index, uint_fast8_t previous_player);
};
