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
            else
            {
                _players_turn = !_players_turn;
            }
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

    auto try_add = [&](int idx)
    {
        if (_board[idx] == 0 && !already_in[idx])
        {
            already_in[idx] = true;
            cached_moves[count++] = idx;
        }
    };

    // get indexes around last move based on directions
    for (const int adj_index : _adjuscent_map[last_move])
    {
        try_add(adj_index);
    }

    // get indexes around enemy_player pieces
    for (const auto &[position, adjacent_indices] : _adjuscent_map)
    {
        if (_board[position] == enemy_player)
        {
            for (const int adj_index : adjacent_indices)
            {
                try_add(adj_index);
            }
        }
    }

    // get indexes around current_player pieces
    for (const auto &[position, adjacent_indices] : _adjuscent_map)
    {
        if (_board[position] == current_player)
        {
            for (const int adj_index : adjacent_indices)
            {
                try_add(adj_index);
            }
        }
    }

    cached_moves.resize(count);
    return cached_moves;
}

int Game::aiTurn()
{
    auto start_time = chrono::high_resolution_clock::now();

    // cout << "Available moves: ";
    // for (const auto& val : _active_square_list) {
    //     int row = val / _board_size + 1;
    //     int col = val % _board_size + 1;
    //     cout << row << " " << col << " | ";
    // }
    // cout << endl;

    // Reduce depth for larger boards to maintain performance
    int depth = min(_win_count + 2, 8); // Cap depth at 8 for performance
    if (_board_size >= 8) depth = min(depth, 6);
    if (_board_size >= 10) depth = min(depth, 4);
    
    _hash_treshold = _win_count - 3;

    if (_active_square_list.empty()) {
        return -1;
    }

    int best_move = -1;
    int best_score = INT_MIN;
    bool found_final = false;

    // Check for immediate winning moves for AI first (threat level 10000)
    if (!found_final) {
        for (int move : _active_square_list) {
            int ai_threat = evaluateMoveThreat(move, 2);
            if (ai_threat >= 10000) { // Immediate win
                best_move = move;
                best_score = 20000; // Highest priority
                found_final = true;
                break;
            }
        }
    }

    // Check for blocking immediate human wins (threat level 10000)
    if (!found_final) {
        for (int move : _active_square_list) {
            int human_threat = evaluateMoveThreat(move, 1);
            if (human_threat >= 10000) { // Human immediate win threat
                best_move = move;
                best_score = 19000; // Second highest priority
                found_final = true;
                break;
            }
        }
    }

    // Check for high-priority threats (level 1000 - one away from win)
    if (!found_final) {
        vector<pair<int, int>> threat_moves; // pair of (move, combined_threat_level)
        threat_moves.reserve(_active_square_list.size()); // Reserve space
        
        for (int move : _active_square_list) {
            int ai_threat = evaluateMoveThreat(move, 2);
            int human_threat = evaluateMoveThreat(move, 1);
            int combined_threat = ai_threat + human_threat; // AI opportunities + blocking value
            
            if (combined_threat >= 1000) {
                threat_moves.emplace_back(move, combined_threat);
            }
        }

        // If we found high-priority threats, pick the best one
        if (!threat_moves.empty()) {
            sort(threat_moves.begin(), threat_moves.end(), 
                 [](const pair<int,int>& a, const pair<int,int>& b) { 
                     return a.second > b.second; 
                 });
            
            best_move = threat_moves[0].first;
            best_score = threat_moves[0].second + 10000; // Third priority level
            found_final = true;
        }
    }

    // Use minimax for strategic evaluation when no immediate threats
    if (!found_final) {
        vector<pair<int, int>> move_scores; // pair of (move, score)
        move_scores.reserve(_active_square_list.size()); // Reserve space
        
        int alpha = _LOSS_SCORE;
        int beta = _WIN_SCORE; // Keep beta high at root level
        
        for (int move : _active_square_list) {
            applyMove(move, 2);
            int score = minimax(depth, false, alpha, beta);
            undoMove(move, 0);

            move_scores.emplace_back(move, score);
            
            if (score > best_score) {
                best_score = score;
                alpha = max(alpha, score); // Update alpha for pruning
                // Don't update beta at root level - we want to evaluate all moves
            }
            
            // At root level, we generally want to evaluate all moves unless we find a guaranteed win
            // Only prune if we found a move that guarantees a win (score near _WIN_SCORE)
            if (score >= _WIN_SCORE - depth) {
                // cout << "Found winning move, stopping search early" << endl;
                break;
            }
        }

        // Find all moves with the best score
        vector<int> best_moves;
        for (const auto& [move, score] : move_scores) {
            if (score == best_score) {
                best_moves.push_back(move);
            }
        }

        // If multiple best moves, choose strategically
        if (best_moves.size() > 1) {
            
            // Find the AI's last move for proximity calculation
            int ai_last_move = -1;
            for (int i = _board_size * _board_size - 1; i >= 0; i--) {
                if (_board[i] == 2) { // Find most recent AI move
                    ai_last_move = i;
                    break;
                }
            }
            
            int chosen_move = best_moves[0]; // Default fallback
            int best_strategic_score = INT_MIN;
            
            for (int move : best_moves) {
                int strategic_score = 0;
                
                // 1. Proximity to AI's last move (if exists)
                if (ai_last_move != -1) {
                    int ai_row = ai_last_move / _board_size;
                    int ai_col = ai_last_move % _board_size;
                    int move_row = move / _board_size;
                    int move_col = move % _board_size;
                    
                    int distance = abs(ai_row - move_row) + abs(ai_col - move_col); // Manhattan distance
                    strategic_score += max(0, 10 - distance); // Closer is better
                }
                
                // 2. Evaluate threat potential for this move
                int ai_threat = evaluateMoveThreat(move, 2);
                int human_threat = evaluateMoveThreat(move, 1);
                strategic_score += ai_threat + (human_threat / 2); // AI threats more important than blocking
                
                // 3. Center preference for early game
                int center_row = _board_size / 2;
                int center_col = _board_size / 2;
                int move_row = move / _board_size;
                int move_col = move % _board_size;
                int center_distance = abs(center_row - move_row) + abs(center_col - move_col);
                strategic_score += max(0, 5 - center_distance); // Prefer center positions
                
                
                if (strategic_score > best_strategic_score) {
                    best_strategic_score = strategic_score;
                    chosen_move = move;
                }
            }
            
            best_move = chosen_move;
        } else if (!best_moves.empty()) {
            best_move = best_moves[0];
        }

    }

    // Apply the best move found
    if (best_move != -1) {
        applyMove(best_move, 2);
    }

    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    // cout << "aiTurn | " << duration.count() << " ms " << endl;

    return best_move;
}

int Game::minimax(int depth, bool maximizingPlayer, int alpha, int beta)
{
    auto tt_it = _trans_table.find(_zobrist_hash);
    if (tt_it != _trans_table.end())
        return tt_it->second;

    if (depth == 0)
        return evalBoard(maximizingPlayer);

    int best_score = maximizingPlayer ? INT_MIN : INT_MAX;
    const bool next_maximizingPlayer = !maximizingPlayer;
    
    const uint_fast8_t current_player = maximizingPlayer ? 2 : 1;
    if (_active_square_list.empty())
        return _TIE_SCORE;

    for (int move : _active_square_list)
    {
        applyMove(move, current_player);
        
        // Check for terminal states right after making the move
        const int check_result = checkBoard(move, current_player);
        int score;
        
        if (check_result == 2) {
            score = _TIE_SCORE; // Tie
        }
        else if (check_result == 1) {
            // If AI (player 2) won, that's good for maximizing player
            // If human (player 1) won, that's bad for maximizing player  
            score = (current_player == 2) ? _WIN_SCORE + depth : _LOSS_SCORE - depth;
        }
        else {
            // Game continues, recurse
            score = minimax(depth - 1, next_maximizingPlayer, alpha, beta);
        }
        
        undoMove(move, 0);

        if (maximizingPlayer)
        {
            if (score > best_score) best_score = score;
            alpha = max(alpha, score);
        }
        else
        {
            if (score < best_score) best_score = score;
            beta = min(beta, score);
        }

        if (beta <= alpha)
            break;
    }

    _trans_table[_zobrist_hash] = best_score;

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
            cout << "Invalid input. Defaulting to AI." << endl;
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
            cout << "Invalid input. Defaulting to 3." << endl;
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

    _board_sep = string(_board_size * 4 - 1, '-');

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
            cout << "Invalid input. Defaulting to " << _board_size << "." << endl;
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
            cout << "Invalid input. Defaulting to 0." << endl;
            player_choice = 0;
        }
    }

    auto result = setSymbols(player_choice);
    while (!result)
    {
        cout << result.error() << endl;
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

inline expected<int, string> Game::playerMove(const uint_fast8_t player_id)
{
    int r, c;

    cout << "\"" << _symbols[player_id] << "\" choose position (row col): ";
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
    applyMove(index, player_id);

    return index;
}

inline void Game::render()
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

inline expected<void, string> Game::setSymbols(int player_choice)
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
    const int N = _board_size * _board_size; // Total number of cells in the board
    _board.clear();                              // Clear the board before resizing
    _board.resize(N, 0); // Add this line!

    generateAdjuscentMap();

    std::mt19937_64 rng(123456789); // Fixed seed for determinism
    std::uniform_int_distribution<zobrist_t> dist;

    _zobrist_table.resize(N);
    for (auto &cell : _zobrist_table)
    {
        for (int i = 0; i < 3; ++i)
        {
            cell[i] = dist(rng);
        }
    }

    _neighbor_count.assign(N, 0);
    _active_square_list.clear();
    _active_square_set.clear();
    _active_square_list.reserve(N);
    _active_square_set.reserve(N);

    _trans_table.clear();
    int trans_N = min(1000000, 1 << min(20, _board_size * _board_size)); // Cap memory usage
    _trans_table.reserve(trans_N);

    _zobrist_hash = 0; // Reset hash
}

// Helper function to evaluate threat level for a specific move and player
inline int Game::evaluateMoveThreat(int move, uint_fast8_t player) const
{
    if (_board[move] != 0) return 0; // Position occupied
    
    const int N = _board_size;
    const int WIN = _win_count;
    int max_threat_level = 0;
    
    int row = move / N;
    int col = move % N;
    
    // Temporarily place piece
    const_cast<vector<uint_fast8_t>&>(_board)[move] = player;
    
    // Check all 4 directions for threats with optimized boundary checking
    static const int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    
    for (int d = 0; d < 4; d++) {
        const int dr = directions[d][0];
        const int dc = directions[d][1];
        int count = 1; // Count the piece we just placed
        
        // Calculate boundary limits once per direction
        int pos_limit, neg_limit;
        if (dr == 0) { // horizontal
            pos_limit = N - col;
            neg_limit = col + 1;
        } else if (dc == 0) { // vertical
            pos_limit = N - row;
            neg_limit = row + 1;
        } else if (dc > 0) { // diagonal (↘)
            pos_limit = min(N - row, N - col);
            neg_limit = min(row + 1, col + 1);
        } else { // diagonal /
            pos_limit = min(N - row, col + 1);
            neg_limit = min(row + 1, N - col);
        }
        
        const int max_steps_pos = min(WIN, pos_limit);
        const int max_steps_neg = min(WIN, neg_limit);
        
        // Count in positive direction with optimized indexing
        for (int step = 1; step < max_steps_pos; step++) {
            const int idx = (row + dr * step) * N + (col + dc * step);
            if (_board[idx] == player) {
                count++;
            } else {
                break;
            }
        }
        
        // Count in negative direction with optimized indexing
        for (int step = 1; step < max_steps_neg; step++) {
            const int idx = (row - dr * step) * N + (col - dc * step);
            if (_board[idx] == player) {
                count++;
            } else {
                break;
            }
        }
        
        // Calculate threat level for this direction
        if (count >= WIN) {
            max_threat_level = 10000; // Immediate win - early exit
            break;
        } else if (count == WIN - 1) {
            max_threat_level = max(max_threat_level, 1000);
        } else if (count == WIN - 2) {
            max_threat_level = max(max_threat_level, 100);
        } else if (count >= 2) {
            max_threat_level = max(max_threat_level, 10);
        }
    }
    
    // Remove the temporary piece
    const_cast<vector<uint_fast8_t>&>(_board)[move] = 0;
    
    return max_threat_level;
}

void Game::generateAdjuscentMap()
{
    const int directions[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}
    };

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
                _adjuscent_map[i].push_back(adj_index);
            }
        }
    }
}

// Fast evaluation: count open lines and proximity to win for both players
inline int Game::evalBoard(const bool maximizingPlayer)
{
    const int ai_player = 2;     // AI is always player 2
    const int human_player = 1;  // Human is always player 1

    int score = 0;
    
    // For larger boards, use a simplified evaluation to maintain performance
    if (_board_size >= 8) {
        // Simplified evaluation for large boards - only check immediate threats
        for (int move : _active_square_list) {
            int ai_threat = evaluateMoveThreat(move, ai_player);
            int human_threat = evaluateMoveThreat(move, human_player);
            
            // Only count high-value threats to reduce computation
            if (ai_threat >= 100) score += ai_threat;
            if (human_threat >= 100) score += human_threat;
        }
    } else {
        // Full evaluation for smaller boards
        for (int move : _active_square_list) {
            int ai_threat = evaluateMoveThreat(move, ai_player);
            int human_threat = evaluateMoveThreat(move, human_player);
            
            // AI opportunities are positive, blocking human threats is also positive
            score += ai_threat + human_threat;
        }
    }

    return maximizingPlayer ? score : -score;
}

inline void Game::markActive(int idx) {
    if (_neighbor_count[idx] > 0 && !_board[idx] && _active_square_set.insert(idx).second) {
        _active_square_list.push_back(idx);
    }
}

inline void Game::unmarkActive(int idx) {
    if (_neighbor_count[idx] == 0 && !_board[idx] && _active_square_set.erase(idx)) {
        _active_square_list.erase(
            std::remove(_active_square_list.begin(), _active_square_list.end(), idx),
            _active_square_list.end());
    }
}

inline void Game::applyMove(int index, uint_fast8_t player) {
    // — update hash & board 
    _zobrist_hash ^= _zobrist_table[index][_board[index]];

    _board[index] = player;

    // 1) Remove the cell itself from active  
    if (_active_square_set.erase(index)) {
        _active_square_list.erase(
          std::remove(_active_square_list.begin(), _active_square_list.end(), index),
          _active_square_list.end());
    }

    // 2) For each neighbor, increment count; if it goes 0→1, mark active
    for (int nb : _adjuscent_map[index]) {
        if (_board[nb] == 0) {
            _neighbor_count[nb]++;
            markActive(nb);
        }
    }

    _zobrist_hash ^= _zobrist_table[index][player];
}

inline void Game::undoMove(int index, uint_fast8_t prev_player) {
    // — update hash & board back —
    _zobrist_hash ^= _zobrist_table[index][_board[index]];
    _board[index] = prev_player;


    // 1) For each neighbor, decrement; if it falls 1→0, unmark active
    for (int nb : _adjuscent_map[index]) {
        if (_board[nb] == 0) {
            _neighbor_count[nb]--;
            unmarkActive(nb);
        }
    }

    // 2) If you’re undoing a real move (making it empty),  
    //    and its neighbors >0, then it itself should be active again:
    if (prev_player == 0) {
        int cnt = 0;
        for (int nb : _adjuscent_map[index])
            if (_board[nb]) ++cnt;

        _neighbor_count[index] = cnt;

        if (cnt > 0) markActive(index);
    }


    _zobrist_hash ^= _zobrist_table[index][prev_player];
}