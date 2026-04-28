#include "game/Game.hpp"
#include <iostream>
#include <random>

using namespace std;

void Game::initialize()
{
    string input;
    int player_choice;

    cout << MOVE_SEPARATOR << endl;
    cout << "Welcome to Tic Tac Toe!" << endl;
    cout << "Enter your move by writing row and column where you wanna place your piece." << endl;
    cout << MOVE_SEPARATOR << endl;

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
        cout << "You are playing against AI." << endl;
    else
        cout << "You are playing against local player." << endl;

    cout << "Select board dimension (3(Default) for 3x3, 4 for 4x4, etc.): ";
    getline(cin, input);

    if (input.empty())
    {
        _board_size = 3;
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
            _board_size = 0;
        }
    }

    setBoardSize();

    _board_separator = string(_board_size * 4 - 1, '-');

    cout << "Select win count greater than 2 and less or equal to " << _board_size << "(Default): ";
    getline(cin, input);

    if (input.empty())
    {
        _win_count = _board_size;
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
                _win_count = 0;
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
                _win_count = _board_size + 1;
            }
        }
        else
        {
            incorrect = false;
        }
        incorrect = _win_count < 3 || _win_count > _board_size;
    }

    _hash_threshold = _win_count - 3;

    cout << "Select your symbol (0(default) = X or 1 = O): ";
    getline(cin, input);

    if (input.empty())
    {
        player_choice = 0;
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
            player_choice = -1;
        }
        result = setSymbols(player_choice);
    }
    cout << "You selected: " << _symbols[1] << endl;
}

void Game::cleanup()
{
    cout << "Cleaning up..." << endl;
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
    const int total_cells = _board_size * _board_size;
    _board.clear();
    _board.resize(total_cells, 0);

    _dm_i = _board_size >= 20 ? 20 :
                (_board_size >= 15 ? 15 :
                (_board_size >= 10 ? 10 : 5));

    generateAdjacentMap();
    precalculateDirectionData();
    precalculateOptimizations();

    std::mt19937_64 random_generator(123456789);
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
    int transposition_table_size = min(1000000, 1 << min(20, _board_size * _board_size));
    _transposition_table.reserve(transposition_table_size);

    _killer_moves.clear();
    _killer_moves.resize(20);
    for (auto &killer_list : _killer_moves)
    {
        killer_list.reserve(2);
    }

    _zobrist_hash = 0;
}