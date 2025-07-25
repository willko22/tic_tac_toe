#include "game/Game.hpp"
#include <iostream>
#include <stdexcept>
#include <sstream>

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

            int index = -1;
            if (_with_ai && !_players_turn) {
                // AI move logic (not implemented in this snippet)
                cout << "AI is making a move..." << endl;
                index =  ai_turn();
            } else {

                expected<int, string> result = player_move(player);
                if (!result) {
                    cout << result.error() << endl;
                    result = player_move(player);
                }
                // Check for win condition here (not implemented in this snippet)
                // cout << player[result.value()] << endl; // Debugging line to check if the position is set correctly
                index = result.value();

            }
            
            loop = checkBoard(index, player);
            

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

int Game::ai_turn() {

    int rand_index = rand() % (_board_size * _board_size);
    while (_player1[rand_index] || _player2[rand_index]) {
        // If the random position is already taken, find another one
        rand_index = rand() % (_board_size * _board_size);
    }

    _player2[rand_index] = true; // AI places its symbol
    // Placeholder for AI logic, which should return an index
    cout << "AI placed its symbol at position: " << (rand_index / _board_size + 1) << " " << (rand_index % _board_size + 1) << endl;


    return rand_index;
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

bool Game::checkBoard(int index, vector<bool>& player) {
    
    int row = index / _board_size;
    int col = index % _board_size;
    
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
        
        // Check in positive direction
        int max_steps_pos = min(_win_count - 1, min((_board_size - 1 - row) / max(1, dr), (_board_size - 1 - col) / max(1, dc)));
        for (int step = 1; step <= max_steps_pos; step++) {
            // new row | new col | count pointer | player identifier
            if (shouldBreak((row + dr * step), (col + dc * step), &count, player)) {
                break; // Stop if we hit a different symbol or out of bounds
            }
            
        }
        
        // Check in negative direction
        int max_steps_neg = min(_win_count - 1, min(row / max(1, -dr), col / max(1, -dc)));
        for (int step = 1; step <= max_steps_neg; step++) {
            // new row | new col | count pointer | player identifier
            if (shouldBreak((row - dr * step), (col - dc * step), &count, player)) {
                break; // Stop if we hit a different symbol or out of bounds
            }
        }

        // cout << count << endl;
        
        // Check if we have enough in a row
        if (count >= _win_count) {
            cout << "Player " << _symbols[_players_turn ? 1 : 2] << " wins!" << endl;
            return false; // End game
        }
    }
    
    // Check for tie (board full). bit or operation on vector<bool> to check if all positions are filled
    bool board_full = true;
    for (size_t i = 0; i < _player1.size(); i++) {
        if (!_player1[i] && !_player2[i]) {  // OR operation: if neither player occupies this position
            board_full = false;
            break;
        }
    }
    
    if (board_full) {
        cout << "It's a tie!" << endl;
        return false; // Tie condition, no empty spaces left
    }
    
    return true; // Game continues
}


bool Game::shouldBreak(int new_row, int new_col, int* count, vector<bool>& player) {
            
    int index = new_row * _board_size + new_col;
    if (player[index]) {
        (*count)++;
    } else {
        return true; // Stop if we hit a different symbol
    }

    return false; // Continue checking
}


expected<int, string> Game::player_move(vector<bool>& player) {
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
    
    if (_player1[index] || _player2[index]) {
        return unexpected("Position already taken. Please choose another position.");
    }
    
    // Place the symbol
    player[index] = true;
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

    // if board already correct size reset it
    if (_player1.size() == _board_size * _board_size) {
        _player1.assign(_board_size * _board_size, 0);
        _player2.assign(_board_size * _board_size, 0);
    }
}

void Game::swapPlayers() {
    _players_turn = !_players_turn; // Toggle the player's turn
}