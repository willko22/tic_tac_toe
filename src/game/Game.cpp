#include "game/Game.hpp"
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <ranges>
#include <climits>
#include <cmath>
#include <chrono>
#include "utils/my_math.hpp"

using namespace std;

Game::Game() : _running(false) {}
Game::~Game() {
    cleanup();
}


void Game::run() {

    _running = true;

    while (_running) {

        initialize();

        bool loop = true;
        while (loop) {

            render();
            vector<bool>& player = _players_turn ? _player1 : _player2;

            if (_with_ai && !_players_turn) {
                // AI move logic (not implemented in this snippet)
                cout << "AI is making a move..." << endl;
                _last_move =  aiTurn();
            } else {

                expected<int, string> result = playerMove(player);
                if (!result) {
                    cout << result.error() << endl;
                    result = playerMove(player);
                }
                // Check for win condition here (not implemented in this snippet)
                // cout << player[result.value()] << endl; // Debugging line to check if the position is set correctly
                _last_move = result.value();

            }

            int check = checkBoard(_last_move, player);
            // cout << "Check result: " << check << endl; // Debugging line to check the result of checkBoard
            if (check == 1) {
                cout << "Player " << _symbols[_players_turn ? 1 : 2] << " wins!" << endl;
                loop = false; // End the game loop
            } else if (check == 2) {
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
        if (!input.empty() && (input[0] == 'n' || input[0] == 'N')) {
            _running = false; // Exit the game loop
        } else {
            _players_turn = true; // Reset to player 1's turn
            // board reset not needed because setting board size and symbols will reset the game state
        }
    }   

}


int Game::aiTurn() {
    

    // _ai_board.assign(_ai_vector_size, 0); // Reset AI board
    // populate _ai_board with data from _board in distance of _ai_size... centered on the last move
    
    populateAIBoard();

    auto available_moves = _ai_board 
        | views::enumerate 
        | views::filter([](auto&& pair) { return !get<1>(pair); })
        | views::transform([](auto&& pair) { return static_cast<int>(get<0>(pair)); })
        | ranges::to<vector<int>>();

    
    vector<int> best_moves = {};
    int best_score = INT_MIN;
    for (int ai_move : available_moves) {
        // Convert AI board index to main board index
        int main_board_move = convertAIIndexToMainBoard(ai_move);
        
        // Skip invalid moves (out of bounds)
        if (main_board_move == -1) {
            continue;
        }
        
        placePiece(main_board_move, _player2); // AI's turn
        int score = minimax(_win_count, false, main_board_move);  // After AI moves, it's human's turn (false)
        placePiece(main_board_move, _player2, true); // Undo AI's move
        
        if (score > best_score) { // AI wants to maximize score
            best_score = score; // Update best score
            best_moves = {main_board_move}; // Track the best move
        } else if (score == best_score) {
            best_moves.push_back(main_board_move); // If score is equal, add to best moves
        }

    }

    // Select random move from best moves for variety
    int best_move = best_moves[rand() % best_moves.size()];

    placePiece(best_move, _player2); // AI places its symbol
    
    // DEBUG: Show actual vs expected
    cout << "DEBUG: AI chose index " << best_move << " -> (" << (best_move / _board_size) << "," << (best_move % _board_size) << ")" << endl;
    
    // Placeholder for AI logic, which should return an index
    cout << "AI placed its symbol at position: " << (best_move / _board_size + 1) << " " << (best_move % _board_size + 1) << endl;

    return best_move;
}


void Game::placePiece(int index, vector<bool>& player, bool remove) {
    player[index] = !remove; // Place the piece for the current player
    _board[index] = !remove; // Update the board state
}


int Game::minimax(int depth, bool maximizingPlayer, int last_move){
    // Check for terminal states first (win/tie)
    if (last_move != -1) {  // Only check if there was a last move
        // The player who made the last move is the opposite of the current player
        // If maximizingPlayer is true, then the last move was made by the minimizing player (human)
        // If maximizingPlayer is false, then the last move was made by the maximizing player (AI)
        vector<bool>& last_player = maximizingPlayer ? _player1 : _player2;
        int check_result = checkBoard(last_move, last_player);
        
        if (check_result == 2) {
            return 0;  // Tie
        } else if (check_result == 1) {
            // If last move was by AI (_player2) and AI won, return positive score
            // If last move was by human (_player1) and human won, return negative score
            return maximizingPlayer ? -10 : 10;
        }
    }

    if (depth == 0) {
        return evaluatePosition(); // Evaluate current position
    }

    
    // Use AI board for optimization - only search within the cropped area!
    auto available_moves = _ai_board 
        | views::enumerate 
        | views::filter([](auto&& pair) { return !get<1>(pair); })
        | views::transform([](auto&& pair) { return static_cast<int>(get<0>(pair)); })
        | ranges::to<vector<int>>();


    int best_score = maximizingPlayer ? INT_MIN : INT_MAX;
    
    for (int ai_move : available_moves) {
        // Convert AI board index to main board index for actual placement
        int main_move = convertAIIndexToMainBoard(ai_move);
        
        // Place the piece for the current player
        if (maximizingPlayer) {
            placePiece(main_move, _player2); // AI's turn
            // Update AI board after move
            _ai_board[ai_move] = true; // Mark AI board position as occupied
            
            int score = minimax(depth - 1, false, main_move);
            
            placePiece(main_move, _player2, true); // Undo AI's move
            // Undo AI board update
            _ai_board[ai_move] = false; // Mark AI board position as unoccupied
            
            if (score > best_score) { // AI wants to maximize score
                best_score = score; // Update best score
            }
        } else {
            placePiece(main_move, _player1); // Human's turn
            // Update AI board after move
            _ai_board[ai_move] = true; // Mark AI board position as occupied
            
            int score = minimax(depth - 1, true, main_move);
            
            placePiece(main_move, _player1, true); // Undo human's move
            // Undo AI board update
            _ai_board[ai_move] = false; // Mark AI board position as unoccupied
            
            if (score < best_score) { // Human wants to minimize score
                best_score = score; // Update best score
            }
        }
    }

    return best_score;
}


int Game::evaluatePosition() {
    // Simple evaluation function for non-terminal positions
    // You can make this more sophisticated later
    
    // For now, return 0 (neutral) for ongoing games at depth limit
    // Later you could add heuristics like:
    // - Count potential winning lines
    // - Evaluate center control
    // - Check for threats/opportunities
    
    return 0; // Neutral position
}


void Game::initialize() {
    string input;

    int player_choice;

    cout << _move_sep << endl;
    cout << "Welcome to Tic Tac Toe!" << endl;
    cout << "Enter your move by writing row and column where you wanna place your piece." << endl;
    cout << _move_sep << endl;

    // Initialize the game board and player vectors
    cout << "Against player or AI? (0 = player, 1 = AI): ";
    getline(cin, input);

    if (!input.empty()) {
        try {
            _with_ai = stoi(input);
        } catch (const invalid_argument&) {
            cerr << "Invalid input. Defaulting to AI." << endl;
        }
    }
    
    if (_with_ai) {
        cout << "You are playing against AI." << endl;
    } else {
        cout << "You are playing against local player." << endl;
    }

    // Initialize the game board and player vectors
    cout << "Select board dimension (3(Default) for 3x3, 4 for 4x4, etc.): ";
    getline(cin, input);

    if (input.empty()) {
        _board_size = 3; // Default to 3 if no input
    } else {
        try {
            _board_size = stoi(input);
        } catch (const invalid_argument&) {
            cerr << "Invalid input. Defaulting to 3." << endl;
            _board_size = 3;
        }
    }
    while (_board_size < 3) {
        cout << "Invalid board size. Please enter a number greater than or equal to 3: ";
        getline(cin, input);
        try {
            _board_size = stoi(input);
        } catch (const invalid_argument&) {
            _board_size = 0; // Force another iteration
        }
    }


    // cout << _board_size << endl; // Debugging line to check win count input
    setBoardSize();
    setBoardSep();

 // #### Set win count
    cout << "Select win count greater than 2 and less or equal to " << _board_size << "(Default): ";
    getline(cin, input);

    if (input.empty()) {
        _win_count = _board_size; // Default to 3 if no input
    } else {
        try {
            _win_count = stoi(input);
        } catch (const invalid_argument&) {
            cerr << "Invalid input. Defaulting to " << _board_size << "." << endl;
            _win_count = _board_size;
        }
    }

    bool incorrect = _win_count < 3 || _win_count > _board_size;
    while (incorrect) {
        if(_win_count < 3) {
            cout << "Invalid win count. Please enter a number greater than 2: ";
            getline(cin, input);
            try {
                _win_count = stoi(input);
            } catch (const invalid_argument&) {
                _win_count = 0; // Force another iteration
            }
        } else if (_win_count > _board_size) {
            cout << "Invalid win count. Please enter a number less or equal " << _board_size << ": ";
            getline(cin, input);
            try {
                _win_count = stoi(input);
            } catch (const invalid_argument&) {
                _win_count = _board_size + 1; // Force another iteration
            }
        } else {
            incorrect = false;
        }
        incorrect = _win_count < 3 || _win_count > _board_size;
    }
    // cout << _win_count << endl; // Debugging line to check win count input

    _ai_size = oddifyMinMax(_win_count + 1, 3, _board_size);
    _ai_vector_size = _ai_size * _ai_size; // Set AI board size based on win count
    _ai_board.resize(_ai_vector_size, 0); // Initialize AI board


    cout << "Select your symbol (0(default) = X or 1 = O): ";
    getline(cin, input);
    
    if (input.empty()) {
        player_choice = 0; // Default to 0 if no input
    } else {
        try {
            player_choice = stoi(input);
        } catch (const invalid_argument&) {
            cerr << "Invalid input. Defaulting to 0." << endl;
            player_choice = 0;
        }
    }

    auto result = setSymbols(player_choice);
    while (!result) {
        cerr << result.error() << endl;
        cout << "Select your symbol (0 = X or 1 = O): ";
        string input;
        getline(cin, input);
        try {
            player_choice = stoi(input);
        } catch (const invalid_argument&) {
            player_choice = -1; // Invalid to force another iteration
        }
        result = setSymbols(player_choice);
    }
    cout << "You selected: " << _symbols[1] << endl;

}


void Game::populateAIBoard() {
    _ai_board.assign(_ai_vector_size, false); // Reset AI board
    
    // Get the center position of the last move
    int center_row = _last_move / _board_size;
    int center_col = _last_move % _board_size;
    
    // Calculate starting position to center the AI board on the last move
    // This can go negative or beyond board boundaries - that's intentional!
    int start_row = center_row - _ai_size / 2;
    int start_col = center_col - _ai_size / 2;
    
    for (int i = 0; i < _ai_size; ++i) {
        for (int j = 0; j < _ai_size; ++j) {
            int main_row = start_row + i;
            int main_col = start_col + j;
            int ai_index = i * _ai_size + j;
            
            // Check if this position is within the main board boundaries
            if (main_row >= 0 && main_row < _board_size && 
                main_col >= 0 && main_col < _board_size) {
                // Within bounds - copy the actual board state
                int main_index = main_row * _board_size + main_col;
                _ai_board[ai_index] = _board[main_index];
            } else {
                // Out of bounds - mark as occupied (can't place pieces here)
                _ai_board[ai_index] = true;
            }
        }
    }
}


int Game::convertAIIndexToMainBoard(int ai_index) {
    // Convert AI board index to AI board coordinates
    int ai_row = ai_index / _ai_size;
    int ai_col = ai_index % _ai_size;
    
    // Calculate the center position of the last move
    int center_row = _last_move / _board_size;
    int center_col = _last_move % _board_size;
    
    // Calculate starting position (same logic as populateAIBoard)
    int start_row = center_row - _ai_size / 2;
    int start_col = center_col - _ai_size / 2;
    
    // Convert to main board coordinates
    int main_row = start_row + ai_row;
    int main_col = start_col + ai_col;
    
    // Validate bounds before converting to index
    if (main_row < 0 || main_row >= _board_size || main_col < 0 || main_col >= _board_size) {
        return -1; // Invalid index
    }
    
    // Convert to main board index
    return main_row * _board_size + main_col;
}


int Game::checkBoard(int index, vector<bool>& player) {
    
    int row = index / _board_size;
    int col = index % _board_size;
    int check_index = index; // Initialize check_index to the current index
    
    // Check all 4 directions from the placed piece
    int directions[4][2] = {
        {0, 1},   // horizontal (→)
        {1, 0},   // vertical (↓)
        {1, 1},   // diagonal \ (↘)
        {1, -1}   // diagonal / (↙)
    };
    
    for (int dir = 0; dir < 4; dir++) {
        int dr = directions[dir][0];
        int dc = directions[dir][1];
        int count = 1; // Count the current piece

        // Calculate boundary limits using branchless operations when possible
        int pos_limit, neg_limit;
        if (dr == 0) {  // horizontal (→)
            pos_limit = _board_size - col;
            neg_limit = col + 1;
        } else if (dc == 0) {  // vertical (↓)
            pos_limit = _board_size - row;
            neg_limit = row + 1;
        } else if (dc > 0) {  // diagonal (↘)
            pos_limit = min(_board_size - row, _board_size - col);
            neg_limit = min(row + 1, col + 1);
        } else {  // diagonal (↙)
            pos_limit = min(_board_size - row, col + 1);
            neg_limit = min(row + 1, _board_size - col);
        }
        
        int max_steps_pos = min(_win_count, pos_limit);
        int max_steps_neg = min(_win_count, neg_limit);

        // Check positive direction - simple and fast
        for (int step = 1; step < max_steps_pos; step++) {
            check_index = (row + dr * step) * _board_size + (col + dc * step);
            if (player[check_index]) {
                count++;
            } else {
                break; // Early exit on first empty/opponent piece
            }
        }
        
        // Check negative direction - simple and fast
        for (int step = 1; step < max_steps_neg; step++) {
            check_index = (row - dr * step) * _board_size + (col - dc * step);
            if (player[check_index]) {
                count++;
            } else {
                break; // Early exit on first empty/opponent piece
            }
        }

        
        // Check if we have enough in a row
        if (count >= _win_count) {
            return 1; // End game
        }
    }
    
    // Check for tie (board full) - using ranges for clarity
    if (ranges::all_of(_board, [](bool used) { return used; })) {
        return 2; // Tie condition, no empty spaces left
    }
    
    return 0; // Game continues
}


expected<int, string> Game::playerMove(vector<bool>& player) {
    int r, c;

    cout << "\"" << _symbols[_players_turn ? 1 : 2] << "\" choose position (row col): ";
    string input;
    getline(cin, input);
    
    // Parse row and column from input
    istringstream iss(input);
    if (!(iss >> r >> c)) {
        return unexpected("Invalid input format. Please enter row and column separated by space.");
    }

    if (c < 1 || r < 1 || c > _board_size || r > _board_size) {
        return unexpected("Invalid position. Please choose a numbers between 1 and " + to_string(_board_size) + ".");
    }

    c--; // Convert to 0-based index
    r--;
    
    // Convert 2D coordinates to 1D index
    int index = r * _board_size + c;
    
    if (_board[index]) {
        return unexpected("Position already taken. Please choose another position.");
    }
    
    // Place the symbol
    placePiece(index, player);
    return index;
}


void Game::render() {

    for (int i = 0; i < _board_size * _board_size; i++) {
        int player_ident = _player1[i] ? 1 : (_player2[i] ? 2 : 0);
        // cout << _player1[i] << player_ident << _player2[i];
        cout << " " << _symbols[player_ident] << " ";
        if ((i + 1) % _board_size == 0) {
            cout << endl;
            if (i < _board_size * _board_size - 1){
                cout << _board_sep << endl;
            } else {
                cout << endl;
            }
        } else {
            cout << "|";
        }
    }

    // Rendering code
}


void Game::cleanup() {
    cout << "Cleaning up..." << endl;
    // Cleanup resources - no need to delete _board as vector manages its own memory
}


void Game::setBoardSep() {
    _board_sep = string(_board_size * 4 - 1, '-');
}


expected<void, string> Game::setSymbols(int player_choice) {

    if (player_choice != 0 && player_choice != 1) {
        return unexpected("Invalid player choice, must be 0 or 1.");
    }

    if (player_choice == 0) {
        _symbols[1] = 'X';
        _symbols[2] = 'O';
    } else if (player_choice == 1) {
        _symbols[1] = 'O';
        _symbols[2] = 'X';
    }

    return {};
}


void Game::setBoardSize() {
    _player1.resize(_board_size * _board_size, 0);
    _player2.resize(_board_size * _board_size, 0);
    _board.resize(_board_size * _board_size, 0);  // Add this line!

    // if board already correct size reset it
    if (_player1.size() == _board_size * _board_size) {
        _player1.assign(_board_size * _board_size, 0);
        _player2.assign(_board_size * _board_size, 0);
        _board.assign(_board_size * _board_size, 0);  // Add this line too!
    }
}


void Game::swapPlayers() {
    _players_turn = !_players_turn; // Toggle the player's turn
}