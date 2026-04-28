#include "game/Game.hpp"
#include <algorithm>
#include <cmath>

using namespace std;

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

    static constexpr int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};

    for (int position = 0; position < total_cells; ++position)
    {
        const int row = position / _board_size;
        const int col = position % _board_size;

        for (int dir = 0; dir < DIRECTION_COUNT; ++dir)
        {
            const int delta_row = directions[dir][0];
            const int delta_col = directions[dir][1];

            const int step_offset = delta_row * _board_size + delta_col;

            int positive_limit, negative_limit;
            if (delta_row == 0)
            {
                positive_limit = _board_size - col;
                negative_limit = col + 1;
            }
            else if (delta_col == 0)
            {
                positive_limit = _board_size - row;
                negative_limit = row + 1;
            }
            else if (delta_col > 0)
            {
                positive_limit = min(_board_size - row, _board_size - col);
                negative_limit = min(row + 1, col + 1);
            }
            else
            {
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

    _temp_board.resize(total_cells, 0);

    _line_positions.resize(total_cells * DIRECTION_COUNT);

    for (int position = 0; position < total_cells; ++position)
    {
        for (int dir = 0; dir < DIRECTION_COUNT; ++dir)
        {
            const int index = position * DIRECTION_COUNT + dir;
            const auto &dir_data = _direction_data[position][dir];

            _line_positions[index].reserve(_win_count * 2);

            for (int step = dir_data.negative_limit - 1; step >= 1; step--)
            {
                _line_positions[index].push_back(position - step * dir_data.step_offset);
            }

            _line_positions[index].push_back(position);

            for (int step = 1; step < dir_data.positive_limit; step++)
            {
                _line_positions[index].push_back(position + step * dir_data.step_offset);
            }
        }
    }

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