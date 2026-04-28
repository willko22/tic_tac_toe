#include "game/Game.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace std;

void Game::markActive(int index)
{
    if (index < 0 || index >= static_cast<int>(_board.size()))
        return;

    if (index >= static_cast<int>(_neighbor_count.size()))
        return;

    if (_neighbor_count[index] > 0 && !_board[index] && _active_square_set.insert(index).second)
    {
        _active_square_list.push_back(index);
    }
}

void Game::unmarkActive(int index)
{
    if (index < 0 || index >= static_cast<int>(_board.size()))
        return;

    if (_neighbor_count[index] == 0 && !_board[index] && _active_square_set.erase(index))
    {
        _active_square_list.erase(
            std::remove(_active_square_list.begin(), _active_square_list.end(), index),
            _active_square_list.end());
    }
}

void Game::applyMove(int index, uint_fast8_t player)
{
    if (index < 0 || index >= static_cast<int>(_board.size()))
    {
        cout << "ERROR: Invalid board index in applyMove: " << index
             << " (board size: " << _board.size() << ", board_size param: "
             << _board_size << "x" << _board_size << ")" << endl;
        throw std::out_of_range("Invalid board index in applyMove: " + std::to_string(index));
    }

    _zobrist_hash ^= _zobrist_table[index][_board[index]];

    _board[index] = player;

    if (_active_square_set.erase(index))
    {
        _active_square_list.erase(
            std::remove(_active_square_list.begin(), _active_square_list.end(), index),
            _active_square_list.end());
    }

    auto it = _adjacent_map.find(index);
    if (it != _adjacent_map.end())
    {
        for (int neighbor : it->second)
        {
            if (neighbor < 0 || neighbor >= static_cast<int>(_board.size()))
            {
                cout << "ERROR: Invalid neighbor index " << neighbor << " from position " << index << endl;
                continue;
            }

            if (_board[neighbor] == 0)
            {
                _neighbor_count[neighbor]++;
                markActive(neighbor);
            }
        }
    }

    _zobrist_hash ^= _zobrist_table[index][player];
}

void Game::undoMove(int index, uint_fast8_t previous_player)
{
    if (index < 0 || index >= static_cast<int>(_board.size()))
    {
        throw std::out_of_range("Invalid board index in undoMove: " + std::to_string(index));
    }

    _zobrist_hash ^= _zobrist_table[index][_board[index]];
    _board[index] = previous_player;

    auto it = _adjacent_map.find(index);
    if (it != _adjacent_map.end())
    {
        for (int neighbor : it->second)
        {
            if (neighbor >= 0 && neighbor < static_cast<int>(_board.size()) && _board[neighbor] == 0)
            {
                _neighbor_count[neighbor]--;
                unmarkActive(neighbor);
            }
        }
    }

    if (previous_player == 0)
    {
        int count = 0;
        auto it2 = _adjacent_map.find(index);
        if (it2 != _adjacent_map.end())
        {
            for (int neighbor : it2->second)
                if (neighbor >= 0 && neighbor < static_cast<int>(_board.size()) && _board[neighbor])
                    ++count;
        }

        _neighbor_count[index] = count;

        if (count > 0)
            markActive(index);
    }

    _zobrist_hash ^= _zobrist_table[index][previous_player];

    auto it_cleanup = _active_square_list.begin();
    while (it_cleanup != _active_square_list.end()) {
        if (*it_cleanup < 0 || *it_cleanup >= static_cast<int>(_board.size()) || _board[*it_cleanup] != 0) {
            _active_square_set.erase(*it_cleanup);
            it_cleanup = _active_square_list.erase(it_cleanup);
        } else {
            ++it_cleanup;
        }
    }
}