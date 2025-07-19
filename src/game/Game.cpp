#include "game/Game.hpp"
#include <iostream>
#include <stdexcept>
#include <tuple>
#include <array>

using namespace std;

Game::Game() : _running(false) {
    initialize();

}

Game::~Game() {
    cleanup();
}

void Game::run() {
    _running = true;

    int player_choice;

    cout << _move_sep << endl;
    cout << "Welcome to Tic Tac Toe!" << endl;
    cout << "Enter your move by writing row and column where you wanna place your piece." << endl;
    cout << _move_sep << endl;
    cout << "Select board dimension (3 for 3x3, 4 for 4x4, etc.): ";
    cin >> _board_size;
    while (_board_size < 3) {
        cout << "\nInvalid board size. Please enter a number greater than or equal to 3: ";
        cin >> _board_size;
        if (_board_size >= 3) {
            cout << endl;
        }
    }

    // cout << _board_size << endl; // Debugging line to check win count input
    setBoardSize();
    setBoardSep();

    cout << "Select win count greater than 2 and less or equal to board size: ";
    cin >> _win_count;
    bool incorrect = _win_count < 3 || _win_count > _board_size;
    while (incorrect) {
        if(_win_count < 3) {
            cout << "\nInvalid win count. Please enter a number greater than 2: ";
            cin >> _win_count;
        } else if (_win_count > _board_size) {
            cout << "\nInvalid win count. Please enter a number less or equal board size: ";
            cin >> _win_count;
        } else {
            incorrect = false;
            cout << endl;

        }
    }

    // cout << _win_count << endl; // Debugging line to check win count input

    cout << "Select your symbol (0 = X or 1 = O): ";
    cin >> player_choice;
    cout << endl;

    auto result = setSymbols(player_choice);
    while (!result) {
        cerr << result.error() << endl;
        cout << "Select your symbol (0 = X or 1 = O): " << endl;
        cin >> player_choice;
        auto result = setSymbols(player_choice);
    }

    cout << "You selected: " << _symbols[1] << endl;

    while (_running) {

        render();
        vector<bool>& player = _players_turn ? _player1 : _player2;

        
        expected<int, string> result = player_move(player);
        if (!result) {
            cout << result.error() << endl;
            result = player_move(player);
        }

        
        // Check for win condition here (not implemented in this snippet)
        cout << player[result.value()] << endl; // Debugging line to check if the position is set correctly
        _running = checkBoard(result.value(), player);
        

        swapPlayers();
    
        
    }
    render();
    
    cout << "Game Ended." << endl;
}



void Game::initialize() {
    cout << "Initializing game..." << endl;
    // Initialize SDL, SFML, or other libraries here
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
    int r,c;

    cout << "\"" << _symbols[_players_turn ? 1 : 2] << "\" choose position:";
    cin >> r;
    cin >> c;
    cout << endl;

    if (c < 1 || r < 1 || c > _board_size || r > _board_size) {
        return unexpected("Invalid position. Please choose a number between 1 and " + to_string(_board_size) + ".");
    }

    c--; // Convert to 0-based index
    r--;
    
    // Convert 2D coordinates to 1D index
    int index = r * _board_size + c;
    
    if (_player1[index] || _player2[index]) {
        return unexpected("Position already taken. Please choose another position.");
    }
    
    // Place the symbol
    player[index] = 1;
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
    // _board.resize(_board_size * _board_size, 0); // Initialize all cells to empty
}

void Game::swapPlayers() {
    _players_turn = !_players_turn; // Toggle the player's turn
}