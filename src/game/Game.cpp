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
    setBoardSep();
    setBoardSize();

    cout << _move_sep << endl;
    cout << "Welcome to Tic Tac Toe!" << endl;
    cout << "Game Board:" << endl;
    cout << " [1,1] | [1,2] | [1,3] \n" << _board_sep << "\n 4 | 5 | 6 \n" << _board_sep << "\n 7 | 8 | 9 " << endl;
    cout << "Enter your move by selecting a number from the board." << endl;
    cout << _move_sep << endl;
    cout << "Select your symbol (0 = X or 1 = O): " << endl;
    // cin >> player_choice;
    player_choice = 1;

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

        expected<int, string> result = player_move();
        if (!result) {
            cout << result.error() << endl;
            player_move();
        }

        
        // Check for win condition here (not implemented in this snippet)
        _running = checkBoard(result.value());
        

        swapPlayers();
    
        
    }
    render();
    
    cout << "Game Ended." << endl;
}



void Game::initialize() {
    cout << "Initializing game..." << endl;
    // Initialize SDL, SFML, or other libraries here
}

bool Game::checkBoard(int index) {
    int cur_ident = _players_turn ? 1 : 2;
    int row = index / _board_size;
    int col = index % _board_size;

    int col_count = 0;
    int row_count = 0;


    int cur = col;
    while (cur < _board_size * _board_size) {
        if (checkWin(cur, &col_count, cur_ident)) {
            return false;
        }
        cur += _board_size;
    }

    cur = row * _board_size;
    for (int i = 0; i < _board_size; i++) {
    
        if (checkWin(cur + i, &row_count, cur_ident)) {
            return false;
        }
    }

    int start_row = row - min(row, col);
    int start_col = col - min(row, col);
    int diag_lenght = _board_size - max(start_row, start_col);


    if (checkDiagonal(start_row, start_col, diag_lenght, cur_ident, 1)) {
        return false;
    }

    start_row = row - min(row, _board_size - 1 - col);
    start_col = col + min(row, _board_size - 1 - col);
    diag_lenght = min(_board_size - start_row, start_col + 1);

    if (checkDiagonal(start_row, start_col, diag_lenght, cur_ident, -1)) {
        return false;
    }
    

    return true;
}


bool Game::checkWin(int index, int* count, int cur_ident) {

    if (_board[index] == cur_ident) {
        (*count)++;
        if (*count == _win_count) {
            cout << "Player \"" << _symbols[cur_ident] << "\" wins!" << endl;
            return true;
        }
    } else {
        *count = 0; // Reset if a different symbol is found
    }

    return false;
}

bool Game::checkDiagonal(int start_row, int start_col, int diag_lenght,  int cur_ident, int dir) {
    if (_board_size - start_row >= _win_count) {
        int d_count = 0;
        for (int i = 0; i < diag_lenght; i++) {
            int index = (start_row + i) * _board_size + start_col + i * dir;
            if (checkWin(index, &d_count, cur_ident)) {
                return true;
            }
        }
    }
    return false;
}

expected<int, string> Game::player_move() {
    int r,c;

    cout << "\"" << _symbols[_players_turn ? 1 : 2] << "\" choose position:";
    cin >> r;
    cin >> c;
    cout << endl;

    if (c < 1 || r < 1 || c > _board_size || r > _board_size) {
        return unexpected("Invalid position. Please choose a number between 1 and " + to_string(_board_size * _board_size) + ".");
    }

    c--; // Convert to 0-based index
    r--;
    
    // Convert 2D coordinates to 1D index
    int index = r * _board_size + c;
    
    if (_board[index] != 0) {
        return unexpected("Position already taken. Please choose another position.");
    }
    
    // Place the symbol
    _board[index] = _players_turn ? 1 : 2;
    
    return index;
    

}

void Game::render() {

    for (int i = 0; i < _board_size * _board_size; i++) {
        cout << " " << _symbols[_board[i]] << " ";
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
    _board.resize(_board_size * _board_size, 0); // Initialize all cells to empty
}

void Game::swapPlayers() {
    _players_turn = !_players_turn; // Toggle the player's turn
}