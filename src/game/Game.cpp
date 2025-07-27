#include "game/Game.hpp"
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <ranges>
#include <climits>
#include <cmath>
#include <chrono>
#include <random>

#include <unordered_map>

using namespace std;

Game::Game() : _running(false) {}
Game::~Game()
{
    cleanup();
}

void Game::run()
{

    _running = true;

    while (_running)
    {

        initialize();

        bool loop = true;
        while (loop)
        {

            render();
            uint_fast8_t player = _players_turn ? 1 : 2; // Determine current player

            if (_with_ai && !_players_turn)
            {
                // AI move logic (not implemented in this snippet)
                cout << "AI is making a move..." << endl;
                _last_move = aiTurn();
            }
            else
            {

                expected<int, string> result = playerMove(player);
                while (!result)
                {
                    cout << result.error() << endl;
                    result = playerMove(player);
                }
                // Check for win condition here (not implemented in this snippet)
                // cout << player[result.value()] << endl; // Debugging line to check if the position is set correctly
                _last_move = result.value();
            }

            int check = checkBoard(_last_move, player);
            // cout << "Check result: " << check << endl; // Debugging line to check the result of checkBoard
            if (check == 1)
            {
                cout << "Player " << _symbols[_players_turn ? 1 : 2] << " wins!" << endl;
                loop = false; // End the game loop
            }
            else if (check == 2)
            {
                cout << "It's a tie!" << endl;
                loop = false; // End the game loop
            }

            swapPlayers();
        }

        render();

        cout << "New game? (Y/n): ";
        string input;
        getline(cin, input);

        // Default to 'Y' if input is empty, otherwise check first character
        if (!input.empty() && (input[0] == 'n' || input[0] == 'N'))
        {
            _running = false; // Exit the game loop
        }
        else
        {
            _players_turn = true; // Reset to player 1's turn
            // board reset not needed because setting board size and symbols will reset the game state
        }
    }
}

inline vector<int> Game::getAvailableMoves(const bool player_turn, const int last_move)
{
    // Preallocate max possible size
    thread_local vector<int> cached_moves;
    thread_local vector<bool> already_in;

    cached_moves.resize(_board_size * _board_size);
    already_in.assign(_board_size * _board_size, false);

    size_t count = 0;

    const uint_fast8_t current_player = player_turn ? 1 : 2;
    const uint_fast8_t enemy_player = player_turn ? 2 : 1;

    auto try_add = [&](int idx) {
        if (_board[idx] == 0 && !already_in[idx]) {
            already_in[idx] = true;
            cached_moves[count++] = idx;
        }
    };

    // get indexes around last move based on directions
    for (const int adj_index : _adjuscent_map[last_move]) {
        try_add(adj_index);
    }

    // get indexes around enemy_player pieces
    for (const auto &[position, adjacent_indices] : _adjuscent_map) {
        if (_board[position] == enemy_player) {
            for (const int adj_index : adjacent_indices) {
                try_add(adj_index);
            }
        }
    }

    // get indexes around current_player pieces
    for (const auto &[position, adjacent_indices] : _adjuscent_map) {
        if (_board[position] == current_player) {
            for (const int adj_index : adjacent_indices) {
                try_add(adj_index);
            }
        }
    }
    
    cached_moves.resize(count);
    return cached_moves;
}

int Game::aiTurn()
{
    // Get all available moves from the main board
    auto start_time = chrono::high_resolution_clock::now();

    vector<int> available_moves = getAvailableMoves(false, _last_move);
    vector<int> best_moves = {};
    int best_score = INT_MIN;

    int depth = _win_count + 2;
    _hash_treshold = _win_count - 3; // Set hash threshold based on win count

    for (int move : available_moves)
    {
        _board[move] = 2; // Place AI's symbol on the board
        int score = minimax(depth, false, move, INT_MIN, INT_MAX);
        _board[move] = 0; // Place AI's symbol on the board


        if (score > best_score)
        {                        // AI wants to maximize score
            best_score = score;  // Update best score
            best_moves = {move}; // Track the best move
        }
        else if (score == best_score)
        {
            best_moves.push_back(move); // Add to best moves if score is equal
        }
    }

    // Select random move from best moves for variety
    cout << "Best moves count: " << best_moves.size() << endl;
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, best_moves.size() - 1);
    int best_move = best_moves[dist(rng)];
    _board[best_move] = 2; // Place AI's symbol on the board

    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    cout << "aiTurn | " << duration.count() << " ms " << endl;

    return best_move;
}

int Game::minimax(int depth, bool maximizingPlayer, int last_move, int alpha, int beta)
{
    auto tt_it = _trans_table.find(_zobrist_hash);
    if (tt_it != _trans_table.end()) return tt_it->second;

    // Check for terminal states first (win/tie)
    const uint_fast8_t last_player = maximizingPlayer ? 1 : 2;
    const int check_result = checkBoard(last_move, last_player);
    if (check_result == 2) return 0; // Tie
    if (check_result == 1) {
        int result = maximizingPlayer ? -10 - depth : 10 + depth;
        _trans_table[_zobrist_hash] = result;
        return result;
    }
    if (depth == 0) return evalBoard(!maximizingPlayer);

    int best_score = maximizingPlayer ? INT_MIN : INT_MAX;
    const bool next_maximizingPlayer = !maximizingPlayer;
    const uint_fast8_t current_player = maximizingPlayer ? 2 : 1;
    vector<int> available_moves = getAvailableMoves(next_maximizingPlayer, last_move);
    if (available_moves.empty()) return 0;

    for (int move : available_moves)
    {
        applyMove(move, current_player);
        int score = minimax(depth - 1, next_maximizingPlayer, move, alpha, beta);
        undoMove(move, 0);

        if (maximizingPlayer)
        {
            if (score > best_score)
            {
                best_score = score;
                alpha = score;
            }
        }
        else
        {
            if (score < best_score)
            {
                best_score = score;
                beta = score;
            }
        }

        if (beta <= alpha)
            break;
    }
    if (depth <= _hash_treshold) _trans_table[_zobrist_hash] = best_score;

    return best_score;
}

void Game::initialize()
{
    string input;

    int player_choice;

    cout << _move_sep << endl;
    cout << "Welcome to Tic Tac Toe!" << endl;
    cout << "Enter your move by writing row and column where you wanna place your piece." << endl;
    cout << _move_sep << endl;

    // Initialize the game board and player vectors
    cout << "Against player or AI? (0 = player, 1 = AI): ";
    getline(cin, input);

    if (!input.empty())
    {
        try
        {
            _with_ai = stoi(input);
        }
        catch (const invalid_argument &)
        {
            cerr << "Invalid input. Defaulting to AI." << endl;
        }
    }

    if (_with_ai)
    {
        cout << "You are playing against AI." << endl;
    }
    else
    {
        cout << "You are playing against local player." << endl;
    }

    // Initialize the game board and player vectors
    cout << "Select board dimension (3(Default) for 3x3, 4 for 4x4, etc.): ";
    getline(cin, input);

    if (input.empty())
    {
        _board_size = 3; // Default to 3 if no input
    }
    else
    {
        try
        {
            _board_size = stoi(input);
        }
        catch (const invalid_argument &)
        {
            cerr << "Invalid input. Defaulting to 3." << endl;
            _board_size = 3;
        }
    }
    while (_board_size < 3)
    {
        cout << "Invalid board size. Please enter a number greater than or equal to 3: ";
        getline(cin, input);
        try
        {
            _board_size = stoi(input);
        }
        catch (const invalid_argument &)
        {
            _board_size = 0; // Force another iteration
        }
    }

    // cout << _board_size << endl; // Debugging line to check win count input
    setBoardSize();
    setBoardSep();

    // #### Set win count
    cout << "Select win count greater than 2 and less or equal to " << _board_size << "(Default): ";
    getline(cin, input);

    if (input.empty())
    {
        _win_count = _board_size; // Default to 3 if no input
    }
    else
    {
        try
        {
            _win_count = stoi(input);
        }
        catch (const invalid_argument &)
        {
            cerr << "Invalid input. Defaulting to " << _board_size << "." << endl;
            _win_count = _board_size;
        }
    }




    bool incorrect = _win_count < 3 || _win_count > _board_size;
    while (incorrect)
    {
        if (_win_count < 3)
        {
            cout << "Invalid win count. Please enter a number greater than 2: ";
            getline(cin, input);
            try
            {
                _win_count = stoi(input);
            }
            catch (const invalid_argument &)
            {
                _win_count = 0; // Force another iteration
            }
        }
        else if (_win_count > _board_size)
        {
            cout << "Invalid win count. Please enter a number less or equal " << _board_size << ": ";
            getline(cin, input);
            try
            {
                _win_count = stoi(input);
            }
            catch (const invalid_argument &)
            {
                _win_count = _board_size + 1; // Force another iteration
            }
        }
        else
        {
            incorrect = false;
        }
        incorrect = _win_count < 3 || _win_count > _board_size;
    }
    // cout << _win_count << endl; // Debugging line to check win count input

    _hash_treshold = _win_count - 3; // Set hash threshold based on win count



    cout << "Select your symbol (0(default) = X or 1 = O): ";
    getline(cin, input);

    if (input.empty())
    {
        player_choice = 0; // Default to 0 if no input
    }
    else
    {
        try
        {
            player_choice = stoi(input);
        }
        catch (const invalid_argument &)
        {
            cerr << "Invalid input. Defaulting to 0." << endl;
            player_choice = 0;
        }
    }

    auto result = setSymbols(player_choice);
    while (!result)
    {
        cerr << result.error() << endl;
        cout << "Select your symbol (0 = X or 1 = O): ";
        string input;
        getline(cin, input);
        try
        {
            player_choice = stoi(input);
        }
        catch (const invalid_argument &)
        {
            player_choice = -1; // Invalid to force another iteration
        }
        result = setSymbols(player_choice);
    }
    cout << "You selected: " << _symbols[1] << endl;
}

int Game::checkBoard(const int index, const uint_fast8_t player) const
{

    int row = index / _board_size;
    int col = index % _board_size;
    int check_index = index; // Initialize check_index to the current index

    // Check all 4 directions from the placed piece
    int directions[4][2] = {
        {0, 1}, // horizontal (→)
        {1, 0}, // vertical (↓)
        {1, 1}, // diagonal \ (↘)
        {1, -1} // diagonal / (↙)
    };

    for (int dir = 0; dir < 4; dir++)
    {
        int dr = directions[dir][0];
        int dc = directions[dir][1];
        int count = 1; // Count the current piece

        // Calculate boundary limits using branchless operations when possible
        int pos_limit, neg_limit;
        if (dr == 0)
        { // horizontal (→)
            pos_limit = _board_size - col;
            neg_limit = col + 1;
        }
        else if (dc == 0)
        { // vertical (↓)
            pos_limit = _board_size - row;
            neg_limit = row + 1;
        }
        else if (dc > 0)
        { // diagonal (↘)
            pos_limit = min(_board_size - row, _board_size - col);
            neg_limit = min(row + 1, col + 1);
        }
        else
        { // diagonal (↙)
            pos_limit = min(_board_size - row, col + 1);
            neg_limit = min(row + 1, _board_size - col);
        }

        int max_steps_pos = min(_win_count, pos_limit);
        int max_steps_neg = min(_win_count, neg_limit);

        // Check positive direction - simple and fast
        for (int step = 1; step < max_steps_pos; step++)
        {
            check_index = (row + dr * step) * _board_size + (col + dc * step);
            if (_board[check_index] == player)
            {
                count++;
            }
            else
            {
                break; // Early exit on first empty/opponent piece
            }
        }

        // Check negative direction - simple and fast
        for (int step = 1; step < max_steps_neg; step++)
        {
            check_index = (row - dr * step) * _board_size + (col - dc * step);
            if (_board[check_index] == player)
            {
                count++;
            }
            else
            {
                break; // Early exit on first empty/opponent piece
            }
        }

        // Check if we have enough in a row
        if (count >= _win_count)
        {
            return 1; // End game
        }
    }

    // Check if the board is full
    for (auto cell : _board)
    {
        if (cell == 0)
        {
            // If we find an empty cell, the game is not over
            return 0;
        }
    }

    // If no win and board is full, it's a tie
    return 2;
}

expected<int, string> Game::playerMove(bool player1)
{
    int r, c;

    cout << "\"" << _symbols[_players_turn ? 1 : 2] << "\" choose position (row col): ";
    string input;
    getline(cin, input);

    // Parse row and column from input
    istringstream iss(input);
    if (!(iss >> r >> c))
    {
        return unexpected("Invalid input format. Please enter row and column separated by space.");
    }

    if (c < 1 || r < 1 || c > _board_size || r > _board_size)
    {
        return unexpected("Invalid position. Please choose a numbers between 1 and " + to_string(_board_size) + ".");
    }

    c--; // Convert to 0-based index
    r--;

    // Convert 2D coordinates to 1D index
    int index = r * _board_size + c;

    if (_board[index])
    {
        return unexpected("Position already taken. Please choose another position.");
    }

    // Place the symbol

    _board[index] = player1 ? 1 : 2; // Set the board position to the current player's symbol
    return index;
}

void Game::render()
{

    for (int i = 0; i < _board_size * _board_size; i++)
    {
        int player_ident = _board[i];
        // cout << _player1[i] << player_ident << _player2[i];
        cout << " " << _symbols[player_ident] << " ";
        if ((i + 1) % _board_size == 0)
        {
            cout << endl;
            if (i < _board_size * _board_size - 1)
            {
                cout << _board_sep << endl;
            }
            else
            {
                cout << endl;
            }
        }
        else
        {
            cout << "|";
        }
    }

    // Rendering code
}

void Game::cleanup()
{
    cout << "Cleaning up..." << endl;
    // Cleanup resources - no need to delete _board as vector manages its own memory
}

void Game::setBoardSep()
{
    _board_sep = string(_board_size * 4 - 1, '-');
}

expected<void, string> Game::setSymbols(int player_choice)
{

    if (player_choice != 0 && player_choice != 1)
    {
        return unexpected("Invalid player choice, must be 0 or 1.");
    }

    if (player_choice == 0)
    {
        _symbols[1] = 'X';
        _symbols[2] = 'O';
    }
    else if (player_choice == 1)
    {
        _symbols[1] = 'O';
        _symbols[2] = 'X';
    }

    return {};
}

void Game::setBoardSize()
{

    _board.clear(); // Clear the board before resizing
    _board.resize(_board_size * _board_size, 0); // Add this line!

    generateAdjuscentMap();

    std::mt19937_64 rng(123456789); // Fixed seed for determinism
    std::uniform_int_distribution<zobrist_t> dist;

    _zobrist_table.resize(_board_size * _board_size);
    for (auto& cell : _zobrist_table) {
        for (int i = 0; i < 3; ++i) {
            cell[i] = dist(rng);
        }
    }
}

void Game::swapPlayers()
{
    _players_turn = !_players_turn; // Toggle the player's turn
}

void Game::generateAdjuscentMap()
{
    const int directions[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}};

    _adjuscent_map.clear();

    for (int i = 0; i < _board_size * _board_size; ++i)
    {
        int row = i / _board_size;
        int col = i % _board_size;

        for (const auto &[dr, dc] : directions)
        {
            int adj_row = row + dr;
            int adj_col = col + dc;

            // Check bounds
            if (adj_row >= 0 && adj_row < _board_size &&
                adj_col >= 0 && adj_col < _board_size)
            {

                int adj_index = adj_row * _board_size + adj_col;
                _adjuscent_map[i].insert(adj_index);
            }
        }
    }
}

// Improved evaluation: count open lines and proximity to win for both players
inline int Game::evalBoard(const bool player1) const
{
    const int player = player1 ? 1 : 2;
    const int enemy = player1 ? 2 : 1;
    const int N = _board_size;
    const int WIN = _win_count;
    const std::vector<uint_fast8_t>& board = _board;

    int score = 0;

    auto score_line = [&](int pc, int ec, int open_ends) -> int {
        if (pc == WIN - 1 && ec == 0 && open_ends == 2) return 100000;     // Near-win
        if (ec == WIN - 1 && pc == 0 && open_ends == 2) return -90000;     // Block threat

        if (pc > 0 && ec == 0) return (1 << pc) * (open_ends + 1);         // Player potential
        if (ec > 0 && pc == 0) return -(1 << ec) * (open_ends + 1);        // Enemy threat
        return 0;
    };

    // Horizontal
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c <= N - WIN; ++c) {
            int pc = 0, ec = 0;
            for (int k = 0; k < WIN; ++k) {
                int v = board[r * N + (c + k)];
                if (v == player) pc++;
                else if (v == enemy) ec++;
            }
            int open = 0;
            if (c - 1 >= 0 && board[r * N + (c - 1)] == 0) open++;
            if (c + WIN < N && board[r * N + (c + WIN)] == 0) open++;
            score += score_line(pc, ec, open);
        }
    }

    // Vertical
    for (int c = 0; c < N; ++c) {
        for (int r = 0; r <= N - WIN; ++r) {
            int pc = 0, ec = 0;
            for (int k = 0; k < WIN; ++k) {
                int v = board[(r + k) * N + c];
                if (v == player) pc++;
                else if (v == enemy) ec++;
            }
            int open = 0;
            if (r - 1 >= 0 && board[(r - 1) * N + c] == 0) open++;
            if (r + WIN < N && board[(r + WIN) * N + c] == 0) open++;
            score += score_line(pc, ec, open);
        }
    }

    // Diagonal ↘ (Top-left to bottom-right)
    for (int r = 0; r <= N - WIN; ++r) {
        for (int c = 0; c <= N - WIN; ++c) {
            int pc = 0, ec = 0;
            for (int k = 0; k < WIN; ++k) {
                int v = board[(r + k) * N + (c + k)];
                if (v == player) pc++;
                else if (v == enemy) ec++;
            }
            int open = 0;
            if (r - 1 >= 0 && c - 1 >= 0 && board[(r - 1) * N + (c - 1)] == 0) open++;
            if (r + WIN < N && c + WIN < N && board[(r + WIN) * N + (c + WIN)] == 0) open++;
            score += score_line(pc, ec, open);
        }
    }

    // Anti-diagonal ↙ (Top-right to bottom-left)
    for (int r = 0; r <= N - WIN; ++r) {
        for (int c = WIN - 1; c < N; ++c) {
            int pc = 0, ec = 0;
            for (int k = 0; k < WIN; ++k) {
                int v = board[(r + k) * N + (c - k)];
                if (v == player) pc++;
                else if (v == enemy) ec++;
            }
            int open = 0;
            if (r - 1 >= 0 && c + 1 < N && board[(r - 1) * N + (c + 1)] == 0) open++;
            if (r + WIN < N && c - WIN >= 0 && board[(r + WIN) * N + (c - WIN)] == 0) open++;
            score += score_line(pc, ec, open);
        }
    }

    // Clamp score to avoid overflow and scale to a [-100, 100] range
    score = std::clamp(score, -1000000, 1000000);
    score = score / 1000;

    return score;
}


inline void Game::applyMove(int index, uint_fast8_t player) {
    _zobrist_hash ^= _zobrist_table[index][_board[index]]; // Remove old
    _board[index] = player;
    _zobrist_hash ^= _zobrist_table[index][player];        // Add new
}

inline void Game::undoMove(int index, uint_fast8_t prev_player) {
    _zobrist_hash ^= _zobrist_table[index][_board[index]];
    _board[index] = prev_player;
    _zobrist_hash ^= _zobrist_table[index][prev_player];
}

