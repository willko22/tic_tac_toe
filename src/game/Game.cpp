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

// ########## CONSTRUCTORS/DESTRUCTORS ##########

Game::Game() : _running(false) {}

Game::~Game()
{
    cleanup();
}

// ########## PUBLIC INTERFACE ##########

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
            uint_fast8_t player = _players_turn ? 1 : 2; // determine current player

            if (_with_ai && !_players_turn)
            {
                // ai move logic
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
                _last_move = result.value();
            }

            int check = checkBoard(_last_move, player);
            if (check == 1)
            {
                cout << "Player " << _symbols[_players_turn ? 1 : 2] << " wins!" << endl;
                loop = false; // end the game loop
            }
            else if (check == 2)
            {
                cout << "It's a tie!" << endl;
                loop = false; // end the game loop
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

        // default to 'Y' if input is empty, otherwise check first character
        if (!input.empty() && (input[0] == 'n' || input[0] == 'N'))
        {
            _running = false; // exit the game loop
        }
        else
        {
            _players_turn = true; // reset to player 1's turn
            // board reset not needed because setting board size and symbols will reset the game state
        }
    }
}

// ########## AI METHODS ##########

int Game::aiTurn()
{
    auto start_time = chrono::high_resolution_clock::now();

    int move_count = _active_square_list.size();
            
    unordered_map<int, int>& depth_maxes = DEPTH_MAP[_dm_i];
    int depth = move_count > 50 ? depth_maxes[50] :
                (move_count > 30 ? depth_maxes[30] :
                (move_count > 10 ? depth_maxes[10] : depth_maxes[0]));
    depth = min(depth, _win_count + 1);

    // Clear transposition table periodically to prevent memory bloat
    if (_transposition_table.size() > 500000) {
        _transposition_table.clear();
    }

    _hash_threshold = max(depth / 2 - 1, 2);

    if (_active_square_list.empty())
    {
        return -1;
    }

    int best_move = _active_square_list.empty() ? -1 : _active_square_list[0]; // Initialize to first valid move
    int best_score = INT_MIN;
    bool found_final = false;

    cout << "DEBUG: Initial best_move=" << best_move << endl;

    // check for immediate winning moves for AI first (threat level 10000)
    if (!found_final)
    {
        for (int move : _active_square_list)
        {
            int ai_threat = evaluateMoveThreat(move, 2);
            if (ai_threat >= 10000)
            { // immediate win
                best_move = move;
                best_score = 20000; // highest priority
                found_final = true;
                cout << "DEBUG: Found AI winning move=" << best_move << endl;
                break;
            }
        }
    }

    // check for blocking immediate human wins (threat level 10000)
    if (!found_final)
    {
        for (int move : _active_square_list)
        {
            int human_threat = evaluateMoveThreat(move, 1);
            if (human_threat >= 10000)
            { // human immediate win threat
                best_move = move;
                best_score = 19000; // second highest priority
                found_final = true;
                cout << "DEBUG: Found blocking move=" << best_move << endl;
                break;
            }
        }
    }

    // check for high-priority threats (level 1000 - one away from win)
    if (!found_final)
    {
        vector<pair<int, int>> threat_moves;              // pair of (move, combined_threat_level)
        threat_moves.reserve(_active_square_list.size()); // reserve space

        for (int move : _active_square_list)
        {
            int ai_threat = evaluateMoveThreat(move, 2);
            int human_threat = evaluateMoveThreat(move, 1);
            int combined_threat = ai_threat + human_threat; // AI opportunities + blocking value

            if (combined_threat >= 1000)
            {
                threat_moves.emplace_back(move, combined_threat);
            }
        }

        // if we found high-priority threats, pick the best one
        if (!threat_moves.empty())
        {
            sort(threat_moves.begin(), threat_moves.end(),
                 [](const pair<int, int> &a, const pair<int, int> &b)
                 {
                     return a.second > b.second;
                 });

            best_move = threat_moves[0].first;
            best_score = threat_moves[0].second + 10000; // third priority level
            found_final = true;
            cout << "DEBUG: Found threat move=" << best_move << endl;
        }
    }

    cout << "DEBUG: Before minimax, best_move=" << best_move << ", found_final=" << found_final << endl;

    // use minimax for strategic evaluation when no immediate threats
    if (!found_final)
    {
        // pre-allocate for performance
        static thread_local vector<pair<int, int>> move_scores;
        move_scores.clear();
        move_scores.reserve(_active_square_list.size());

        const int alpha_start = LOSS_SCORE;
        const int beta_start = WIN_SCORE;
        int alpha = alpha_start;

        for (int move : _active_square_list)
        {
            applyMove(move, 2);
            const int score = minimax(depth, false, alpha, beta_start);
            undoMove(move, 0);

            move_scores.emplace_back(move, score);

            if (score > best_score)
            {
                best_score = score;
                alpha = max(alpha, score);
            }

            // early termination for guaranteed wins
            if (score >= WIN_SCORE - depth)
            {
                break;
            }
        }

        // find best moves efficiently
        static thread_local vector<int> best_moves;
        best_moves.clear();
        best_moves.reserve(move_scores.size());

        for (const auto &[move, score] : move_scores)
        {
            if (score == best_score)
            {
                best_moves.push_back(move);
            }
        }

        // strategic selection for multiple best moves
        if (best_moves.size() > 1)
        {
            // find AI's most recent move
            int ai_last_move = -1;
            for (int i = _board_size * _board_size - 1; i >= 0; i--)
            {
                if (_board[i] == 2)
                {
                    ai_last_move = i;
                    break;
                }
            }

            int chosen_move = best_moves[0];
            int best_strategic_score = INT_MIN;

            for (int move : best_moves)
            {
                int strategic_score = 0;

                // proximity bonus
                if (ai_last_move != -1)
                {
                    const int ai_row = ai_last_move / _board_size;
                    const int ai_col = ai_last_move % _board_size;
                    const int move_row = move / _board_size;
                    const int move_col = move % _board_size;
                    const int distance = abs(ai_row - move_row) + abs(ai_col - move_col);
                    strategic_score += max(0, 10 - distance);
                }

                // threat evaluation
                const int ai_threat = evaluateMoveThreat(move, 2);
                const int human_threat = evaluateMoveThreat(move, 1);
                strategic_score += ai_threat + (human_threat >> 1); // bit shift for /2

                // center preference using precalculated distances
                strategic_score += max(0, 5 - _center_distances[move]);

                if (strategic_score > best_strategic_score)
                {
                    best_strategic_score = strategic_score;
                    chosen_move = move;
                }
            }

            best_move = chosen_move;
        }
        else if (!best_moves.empty())
        {
            best_move = best_moves[0];
        }
    }

    cout << "DEBUG: Final best_move=" << best_move << endl;

    // apply the best move found
    if (best_move != -1)
    {
        // Validate the move
        if (best_move < 0 || best_move >= static_cast<int>(_board.size()))
        {
            cout << "ERROR: Invalid move index " << best_move << " - out of bounds!" << endl;
            cout << "Active moves: ";
            for (int move : _active_square_list) {
                cout << move << " ";
            }
            cout << endl;
            return -1;
        }
        
        applyMove(best_move, 2);
    }

    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    cout << "AI took " << duration.count() << " ms to decide on move: " << best_move << endl;

    return best_move;
}

int Game::minimax(int depth, bool maximizing_player, int alpha, int beta)
{
    // check transposition table first
    const auto transposition_table_iterator = _transposition_table.find(_zobrist_hash);
    if (transposition_table_iterator != _transposition_table.end())
        return transposition_table_iterator->second;

    // base case: depth limit reached
    if (depth == 0)
        return evalBoard(maximizing_player);

    // base case: no moves available
    if (_active_square_list.empty())
        return TIE_SCORE;

    // copy and order moves for better pruning
    static thread_local vector<int> ordered_moves;
    ordered_moves = _active_square_list;
    orderMoves(ordered_moves, maximizing_player);

    int best_score = maximizing_player ? INT_MIN : INT_MAX;
    const uint_fast8_t current_player = maximizing_player ? 2 : 1;

    for (int move : ordered_moves)
    {
        applyMove(move, current_player);

        // check for terminal states immediately after move
        const int check_result = checkBoard(move, current_player);
        int score;

        if (check_result == 2)
        {
            score = TIE_SCORE;
        }
        else if (check_result == 1)
        {
            // win/loss with depth bonus for faster wins
            score = (current_player == 2) ? WIN_SCORE + depth : LOSS_SCORE - depth;
        }
        else
        {
            // continue search
            score = minimax(depth - 1, !maximizing_player, alpha, beta);
        }

        undoMove(move, 0);

        // update best score and alpha-beta bounds
        if (maximizing_player)
        {
            if (score > best_score)
                best_score = score;
            alpha = max(alpha, score);
        }
        else
        {
            if (score < best_score)
                best_score = score;
            beta = min(beta, score);
        }

        // killer move heuristic - store moves that cause cutoffs
        if (beta <= alpha)
        {
            if (depth < static_cast<int>(_killer_moves.size()) && _killer_moves[depth].size() < 2)
            {
                auto it = find(_killer_moves[depth].begin(), _killer_moves[depth].end(), move);
                if (it == _killer_moves[depth].end())
                {
                    _killer_moves[depth].push_back(move);
                }
            }
            break; // alpha-beta pruning
        }
    }

    // store in transposition table if above threshold
    if (depth >= _hash_threshold)
    {
        _transposition_table[_zobrist_hash] = best_score;
    }

    return best_score;
}

// ########## CORE GAME METHODS ##########

void Game::initialize()
{
    string input;
    int player_choice;

    cout << MOVE_SEPARATOR << endl;
    cout << "Welcome to Tic Tac Toe!" << endl;
    cout << "Enter your move by writing row and column where you wanna place your piece." << endl;
    cout << MOVE_SEPARATOR << endl;

    // initialize the game board and player vectors
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

    // initialize the game board and player vectors
    cout << "Select board dimension (3(Default) for 3x3, 4 for 4x4, etc.): ";
    getline(cin, input);

    if (input.empty())
    {
        _board_size = 3; // default to 3 if no input
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
            _board_size = 0; // force another iteration
        }
    }

    setBoardSize();

    _board_separator = string(_board_size * 4 - 1, '-');

    // set win count
    cout << "Select win count greater than 2 and less or equal to " << _board_size << "(Default): ";
    getline(cin, input);

    if (input.empty())
    {
        _win_count = _board_size; // default to board size if no input
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
                _win_count = 0; // force another iteration
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
                _win_count = _board_size + 1; // force another iteration
            }
        }
        else
        {
            incorrect = false;
        }
        incorrect = _win_count < 3 || _win_count > _board_size;
    }

    _hash_threshold = _win_count - 3; // set hash threshold based on win count

    cout << "Select your symbol (0(default) = X or 1 = O): ";
    getline(cin, input);

    if (input.empty())
    {
        player_choice = 0; // default to 0 if no input
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
            player_choice = -1; // invalid to force another iteration
        }
        result = setSymbols(player_choice);
    }
    cout << "You selected: " << _symbols[1] << endl;
}

// ########## GAME LOGIC ##########

int Game::checkBoard(const int index, const uint_fast8_t player) const
{
    // Use precalculated direction data for optimized win checking
    const auto &direction_info = _direction_data[index];

    // check all 4 directions from the placed piece
    for (int dir = 0; dir < DIRECTION_COUNT; dir++)
    {
        const auto &dir_data = direction_info[dir];
        int count = 1; // count the current piece

        const int max_steps_positive = min(_win_count, dir_data.positive_limit);
        const int max_steps_negative = min(_win_count, dir_data.negative_limit);

        // check positive direction using precalculated step offset
        for (int step = 1; step < max_steps_positive; step++)
        {
            if (_board[index + step * dir_data.step_offset] == player)
            {
                count++;
            }
            else
            {
                break; // early exit on first empty/opponent piece
            }
        }

        // check negative direction using precalculated step offset
        for (int step = 1; step < max_steps_negative; step++)
        {
            if (_board[index - step * dir_data.step_offset] == player)
            {
                count++;
            }
            else
            {
                break; // early exit on first empty/opponent piece
            }
        }

        // check if we have enough in a row
        if (count >= _win_count)
        {
            return 1; // end game
        }
    }

    // check if the board is full
    for (auto cell : _board)
    {
        if (cell == 0)
        {
            // if we find an empty cell, the game is not over
            return 0;
        }
    }

    // if no win and board is full, it's a tie
    return 2;
}

// ########## PLAYER INTERACTION ##########

inline expected<int, string> Game::playerMove(const uint_fast8_t player_id)
{
    int row, column;

    cout << "\"" << _symbols[player_id] << "\" choose position (row col): ";
    string input;
    getline(cin, input);

    // parse row and column from input
    istringstream input_stream(input);
    if (!(input_stream >> row >> column))
    {
        return unexpected("Invalid input format. Please enter row and column separated by space.");
    }

    if (column < 1 || row < 1 || column > _board_size || row > _board_size)
    {
        return unexpected("Invalid position. Please choose a numbers between 1 and " + to_string(_board_size) + ".");
    }

    column--; // convert to 0-based index
    row--;

    // convert 2D coordinates to 1D index
    int index = row * _board_size + column;

    if (_board[index])
    {
        return unexpected("Position already taken. Please choose another position.");
    }

    // place the symbol
    applyMove(index, player_id);

    return index;
}

void Game::render() const
{
    for (int i = 0; i < _board_size * _board_size; i++)
    {
        int player_identifier = _board[i];
        cout << " " << _symbols[player_identifier] << " ";
        if ((i + 1) % _board_size == 0)
        {
            cout << endl;
            if (i < _board_size * _board_size - 1)
            {
                cout << _board_separator << endl;
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
}

void Game::cleanup()
{
    cout << "Cleaning up..." << endl;
    // cleanup resources - no need to delete _board as vector manages its own memory
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
    const int total_cells = _board_size * _board_size; // total number of cells in the board
    _board.clear();                                    // clear the board before resizing
    _board.resize(total_cells, 0);

    _dm_i = _board_size >= 20 ? 20 :
                (_board_size >= 15 ? 15 :
                (_board_size >= 10 ? 10 : 5));

    generateAdjacentMap();
    precalculateDirectionData();
    precalculateOptimizations();

    std::mt19937_64 random_generator(123456789); // fixed seed for determinism
    std::uniform_int_distribution<zobrist_t> distribution;

    _zobrist_table.clear();
    _zobrist_table.resize(total_cells);
    for (auto &cell : _zobrist_table)
    {
        for (int i = 0; i < 3; ++i)
        {
            cell[i] = distribution(random_generator);
        }
    }

    _neighbor_count.assign(total_cells, 0);
    _active_square_list.clear();
    _active_square_set.clear();
    _active_square_list.reserve(total_cells);
    _active_square_set.reserve(total_cells);

    _transposition_table.clear();
    int transposition_table_size = min(1000000, 1 << min(20, _board_size * _board_size)); // cap memory usage
    _transposition_table.reserve(transposition_table_size);

    // initialize killer moves table
    _killer_moves.clear();
    _killer_moves.resize(20); // max depth of 20
    for (auto &killer_list : _killer_moves)
    {
        killer_list.reserve(2); // store up to 2 killer moves per depth
    }

    _zobrist_hash = 0; // reset hash
}

//========== Helper Function for Threat Evaluation ==========
int Game::evaluateMoveThreat(int move, uint_fast8_t player) const
{
    if (_board[move] != 0)
        return 0; // position occupied

    const int win_requirement = _win_count;
    int max_threat_level = 0;

    // Use precalculated direction data for optimized threat evaluation
    const auto &direction_info = _direction_data[move];
    
    for (int dir = 0; dir < DIRECTION_COUNT; dir++)
    {
        const auto &dir_data = direction_info[dir];
        int count = 1; // count the hypothetical piece

        const int max_steps_positive = min(win_requirement, dir_data.positive_limit);
        const int max_steps_negative = min(win_requirement, dir_data.negative_limit);

        // Check positive direction
        for (int step = 1; step < max_steps_positive; step++)
        {
            if (_board[move + step * dir_data.step_offset] == player)
            {
                count++;
            }
            else
            {
                break; // early exit on first empty/opponent piece
            }
        }

        // Check negative direction
        for (int step = 1; step < max_steps_negative; step++)
        {
            if (_board[move - step * dir_data.step_offset] == player)
            {
                count++;
            }
            else
            {
                break; // early exit on first empty/opponent piece
            }
        }

        // Calculate threat level with early exit for immediate wins
        if (count >= win_requirement)
        {
            return 10000; // immediate win found - return immediately
        }
        else if (count == win_requirement - 1)
        {
            max_threat_level = max(max_threat_level, 1000);
        }
        else if (count == win_requirement - 2)
        {
            max_threat_level = max(max_threat_level, 100);
        }
        else if (count >= 2)
        {
            max_threat_level = max(max_threat_level, 10);
        }
    }

    return max_threat_level;
}

//========== Move Ordering for Better Alpha-Beta Pruning ==========
void Game::orderMoves(vector<int> &moves, bool maximizing_player) const
{
    const uint_fast8_t player = maximizing_player ? 2 : 1;
    const uint_fast8_t opponent = maximizing_player ? 1 : 2;

    // sort moves by threat level (highest threat first)
    sort(moves.begin(), moves.end(), [this, player, opponent](int a, int b)
         {
        int a_threat = evaluateMoveThreat(a, player) + evaluateMoveThreat(a, opponent);
        int b_threat = evaluateMoveThreat(b, player) + evaluateMoveThreat(b, opponent);
        
        // prioritize center positions if threats are equal
        if (a_threat == b_threat) {
            return _center_distances[a] < _center_distances[b];
        }
        
        return a_threat > b_threat; });
}

//========== Precalculations ==========
void Game::generateAdjacentMap()
{
    const int directions[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}};

    _adjacent_map.clear();
    _adjacent_map.reserve(_board_size * _board_size);

    for (int i = 0; i < _board_size * _board_size; ++i)
    {
        int row = i / _board_size;
        int col = i % _board_size;

        std::vector<int> neighbors;
        neighbors.reserve(8);

        for (const auto &[delta_row, delta_col] : directions)
        {
            int adjacent_row = row + delta_row;
            int adjacent_col = col + delta_col;

            // check bounds
            if (adjacent_row >= 0 && adjacent_row < _board_size &&
                adjacent_col >= 0 && adjacent_col < _board_size)
            {
                int adjacent_index = adjacent_row * _board_size + adjacent_col;
                neighbors.push_back(adjacent_index);
            }
        }
        _adjacent_map.emplace(i, std::move(neighbors));
    }
}

void Game::precalculateDirectionData()
{
    const int total_cells = _board_size * _board_size;
    _direction_data.resize(total_cells);

    // Direction vectors: horizontal, vertical, diagonal(\), diagonal(/)
    static constexpr int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};

    for (int position = 0; position < total_cells; ++position)
    {
        const int row = position / _board_size;
        const int col = position % _board_size;

        for (int dir = 0; dir < DIRECTION_COUNT; ++dir)
        {
            const int delta_row = directions[dir][0];
            const int delta_col = directions[dir][1];

            // Calculate step offset for this direction
            const int step_offset = delta_row * _board_size + delta_col;

            // Calculate positive and negative limits
            int positive_limit, negative_limit;
            if (delta_row == 0)
            { // horizontal
                positive_limit = _board_size - col;
                negative_limit = col + 1;
            }
            else if (delta_col == 0)
            { // vertical
                positive_limit = _board_size - row;
                negative_limit = row + 1;
            }
            else if (delta_col > 0)
            { // diagonal (\)
                positive_limit = min(_board_size - row, _board_size - col);
                negative_limit = min(row + 1, col + 1);
            }
            else
            { // diagonal (/)
                positive_limit = min(_board_size - row, col + 1);
                negative_limit = min(row + 1, _board_size - col);
            }

            _direction_data[position][dir] = {
                step_offset,
                positive_limit,
                negative_limit};
        }
    }
}

void Game::precalculateOptimizations()
{
    const int total_cells = _board_size * _board_size;

    // Initialize temp board for threat evaluation
    _temp_board.resize(total_cells, 0);

    // Precalculate all line positions for each cell and direction
    _line_positions.resize(total_cells * DIRECTION_COUNT);

    for (int position = 0; position < total_cells; ++position)
    {
        for (int dir = 0; dir < DIRECTION_COUNT; ++dir)
        {
            const int index = position * DIRECTION_COUNT + dir;
            const auto &dir_data = _direction_data[position][dir];

            _line_positions[index].reserve(_win_count * 2); // max possible line length

            // Add positions in negative direction
            for (int step = dir_data.negative_limit - 1; step >= 1; step--)
            {
                _line_positions[index].push_back(position - step * dir_data.step_offset);
            }

            // Add current position
            _line_positions[index].push_back(position);

            // Add positions in positive direction
            for (int step = 1; step < dir_data.positive_limit; step++)
            {
                _line_positions[index].push_back(position + step * dir_data.step_offset);
            }
        }
    }

    // Precalculate center distances
    _center_distances.resize(total_cells);
    const int center_row = _board_size >> 1;
    const int center_col = _board_size >> 1;

    for (int position = 0; position < total_cells; ++position)
    {
        const int row = position / _board_size;
        const int col = position % _board_size;
        _center_distances[position] = abs(center_row - row) + abs(center_col - col);
    }
}

//========== Fast Board Evaluation ==========
int Game::evalBoard(const bool maximizing_player) const
{
    static constexpr int AI_PLAYER = 2;
    static constexpr int HUMAN_PLAYER = 1;

    int score = 0;

    // clear threat cache for new evaluation
    _threat_cache.clear();

    // for very large boards, use extremely simplified evaluation
    if (_board_size >= 15)
    {
        // only evaluate immediate threats for very large boards
        for (int move : _active_square_list)
        {
            auto it = _threat_cache.find(move);
            int ai_threat, human_threat;

            if (it != _threat_cache.end())
            {
                ai_threat = it->second.first;
                human_threat = it->second.second;
            }
            else
            {
                ai_threat = evaluateMoveThreat(move, AI_PLAYER);
                human_threat = evaluateMoveThreat(move, HUMAN_PLAYER);
                _threat_cache[move] = {ai_threat, human_threat};
            }

            // only count critical threats (>= 1000) to minimize computation
            if (ai_threat >= 1000)
                score += ai_threat;
            if (human_threat >= 1000)
                score -= human_threat;
        }
    }
    else if (_board_size >= 8)
    {
        // only evaluate high-value threats for large boards
        for (int move : _active_square_list)
        {
            auto it = _threat_cache.find(move);
            int ai_threat, human_threat;

            if (it != _threat_cache.end())
            {
                ai_threat = it->second.first;
                human_threat = it->second.second;
            }
            else
            {
                ai_threat = evaluateMoveThreat(move, AI_PLAYER);
                human_threat = evaluateMoveThreat(move, HUMAN_PLAYER);
                _threat_cache[move] = {ai_threat, human_threat};
            }

            // only count significant threats (>= 100) to reduce computation
            if (ai_threat >= 100)
                score += ai_threat;
            if (human_threat >= 100)
                score -= human_threat;
        }
    }
    else
    {
        // full evaluation for smaller boards
        for (int move : _active_square_list)
        {
            auto it = _threat_cache.find(move);
            int ai_threat, human_threat;

            if (it != _threat_cache.end())
            {
                ai_threat = it->second.first;
                human_threat = it->second.second;
            }
            else
            {
                ai_threat = evaluateMoveThreat(move, AI_PLAYER);
                human_threat = evaluateMoveThreat(move, HUMAN_PLAYER);
                _threat_cache[move] = {ai_threat, human_threat};
            }

            score += ai_threat - human_threat;
        }
    }

    return maximizing_player ? score : -score;
}

// ########## BOARD STATE MANAGEMENT ##########

inline void Game::markActive(int index)
{
    if (_neighbor_count[index] > 0 && !_board[index] && _active_square_set.insert(index).second)
    {
        _active_square_list.push_back(index);
    }
}

inline void Game::unmarkActive(int index)
{
    if (_neighbor_count[index] == 0 && !_board[index] && _active_square_set.erase(index))
    {
        _active_square_list.erase(
            std::remove(_active_square_list.begin(), _active_square_list.end(), index),
            _active_square_list.end());
    }
}

inline void Game::applyMove(int index, uint_fast8_t player)
{
    // Bounds checking
    if (index < 0 || index >= static_cast<int>(_board.size()))
    {
        throw std::out_of_range("Invalid board index in applyMove: " + std::to_string(index));
    }

    // update hash & board
    _zobrist_hash ^= _zobrist_table[index][_board[index]];

    _board[index] = player;

    // 1) remove the cell itself from active
    if (_active_square_set.erase(index))
    {
        _active_square_list.erase(
            std::remove(_active_square_list.begin(), _active_square_list.end(), index),
            _active_square_list.end());
    }

    // 2) for each neighbor, increment count; if it goes 0→1, mark active
    for (int neighbor : _adjacent_map.at(index))
    {
        if (_board[neighbor] == 0)
        {
            _neighbor_count[neighbor]++;
            markActive(neighbor);
        }
    }

    _zobrist_hash ^= _zobrist_table[index][player];
}

inline void Game::undoMove(int index, uint_fast8_t previous_player)
{
    // update hash & board back
    _zobrist_hash ^= _zobrist_table[index][_board[index]];
    _board[index] = previous_player;

    // 1) for each neighbor, decrement; if it falls 1→0, unmark active
    for (int neighbor : _adjacent_map.at(index))
    {
        if (_board[neighbor] == 0)
        {
            _neighbor_count[neighbor]--;
            unmarkActive(neighbor);
        }
    }

    // 2) if you're undoing a real move (making it empty),
    //    and its neighbors >0, then it itself should be active again:
    if (previous_player == 0)
    {
        int count = 0;
        for (int neighbor : _adjacent_map[index])
            if (_board[neighbor])
                ++count;

        _neighbor_count[index] = count;

        if (count > 0)
            markActive(index);
    }

    _zobrist_hash ^= _zobrist_table[index][previous_player];
}

// //########## DEPRECATED FUNCTION ##########
// inline vector<int> Game::getAvailableMoves(const bool player_turn, const int last_move)
// {
//     // Preallocate max possible size
//     thread_local vector<int> cached_moves;
//     thread_local vector<bool> already_in;

//     cached_moves.resize(_board_size * _board_size);
//     already_in.assign(_board_size * _board_size, false);

//     size_t count = 0;

//     const uint_fast8_t current_player = player_turn ? 1 : 2;
//     const uint_fast8_t enemy_player = player_turn ? 2 : 1;

//     auto try_add = [&](int idx)
//     {
//         if (_board[idx] == 0 && !already_in[idx])
//         {
//             already_in[idx] = true;
//             cached_moves[count++] = idx;
//         }
//     };

//     // get indexes around last move based on directions
//     for (const int adj_index : _adjuscent_map[last_move])
//     {
//         try_add(adj_index);
//     }

//     // get indexes around enemy_player pieces
//     for (const auto &[position, adjacent_indices] : _adjuscent_map)
//     {
//         if (_board[position] == enemy_player)
//         {
//             for (const int adj_index : adjacent_indices)
//             {
//                 try_add(adj_index);
//             }
//         }
//     }

//     // get indexes around current_player pieces
//     for (const auto &[position, adjacent_indices] : _adjuscent_map)
//     {
//         if (_board[position] == current_player)
//         {
//             for (const int adj_index : adjacent_indices)
//             {
//                 try_add(adj_index);
//             }
//         }
//     }

//     cached_moves.resize(count);
//     return cached_moves;
// }
