#include "game/Game.hpp"
#include <iostream>
#include <sstream>

using namespace std;

int Game::checkBoard(const int index, const uint_fast8_t player) const
{
    if (index < 0 || index >= static_cast<int>(_board.size()))
        return 0;

    const auto &direction_info = _direction_data[index];

    for (int dir = 0; dir < DIRECTION_COUNT; dir++)
    {
        const auto &dir_data = direction_info[dir];
        int count = 1;

        const int max_steps_positive = min(_win_count, dir_data.positive_limit);
        const int max_steps_negative = min(_win_count, dir_data.negative_limit);

        for (int step = 1; step < max_steps_positive; step++)
        {
            if (_board[index + step * dir_data.step_offset] == player)
                count++;
            else
                break;
        }

        for (int step = 1; step < max_steps_negative; step++)
        {
            if (_board[index - step * dir_data.step_offset] == player)
                count++;
            else
                break;
        }

        if (count >= _win_count)
            return 1;
    }

    for (auto cell : _board)
    {
        if (cell == 0)
            return 0;
    }

    return 2;
}

expected<int, string> Game::playerMove(const uint_fast8_t player_id)
{
    int row, column;

    cout << '"' << _symbols[player_id] << '"' << " choose position (row col): ";
    string input;
    getline(cin, input);

    istringstream input_stream(input);
    if (!(input_stream >> row >> column))
    {
        return unexpected("Invalid input format. Please enter row and column separated by space.");
    }

    if (column < 1 || row < 1 || column > _board_size || row > _board_size)
    {
        return unexpected("Invalid position. Please choose a numbers between 1 and " + to_string(_board_size) + ".");
    }

    column--;
    row--;

    int index = row * _board_size + column;

    if (_board[index])
    {
        return unexpected("Position already taken. Please choose another position.");
    }

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