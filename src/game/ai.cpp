#include "game/Game.hpp"
#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <iostream>

using namespace std;

int Game::aiTurn()
{
    auto start_time = chrono::high_resolution_clock::now();

    {
        auto it = _active_square_list.begin();
        while (it != _active_square_list.end()) {
            if (*it < 0 || *it >= static_cast<int>(_board.size()) || _board[*it] != 0) {
                cout << "WARNING: Removing invalid index from active list: " << *it << endl;
                _active_square_set.erase(*it);
                it = _active_square_list.erase(it);
            } else {
                ++it;
            }
        }
    }

    int move_count = _active_square_list.size();

    unordered_map<int, int>& depth_maxes = DEPTH_MAP[_dm_i];
    int depth = move_count > 50 ? depth_maxes[50] :
                (move_count > 30 ? depth_maxes[30] :
                (move_count > 10 ? depth_maxes[10] : depth_maxes[0]));
    depth = min(depth, _win_count + 1);

    if (_transposition_table.size() > 500000) {
        _transposition_table.clear();
    }

    _hash_threshold = max(depth / 2 - 1, 2);

    if (_active_square_list.empty())
    {
        return -1;
    }

    int best_move = _active_square_list.empty() ? -1 : _active_square_list[0];
    int best_score = INT_MIN;
    bool found_final = false;

    if (!found_final)
    {
        for (int move : _active_square_list)
        {
            if (move < 0 || move >= static_cast<int>(_board.size()) || _board[move] != 0)
                continue;

            int ai_threat = evaluateMoveThreat(move, 2);
            if (ai_threat >= 10000)
            {
                best_move = move;
                best_score = 20000;
                found_final = true;
                break;
            }
        }
    }

    if (!found_final)
    {
        for (int move : _active_square_list)
        {
            if (move < 0 || move >= static_cast<int>(_board.size()) || _board[move] != 0)
                continue;

            int human_threat = evaluateMoveThreat(move, 1);
            if (human_threat >= 10000)
            {
                best_move = move;
                best_score = 19000;
                found_final = true;
                cout << "DEBUG: Found blocking move=" << best_move << endl;
                break;
            }
        }
    }

    if (!found_final)
    {
        vector<pair<int, int>> threat_moves;
        threat_moves.reserve(_active_square_list.size());

        for (int move : _active_square_list)
        {
            if (move < 0 || move >= static_cast<int>(_board.size()) || _board[move] != 0)
                continue;

            int ai_threat = evaluateMoveThreat(move, 2);
            int human_threat = evaluateMoveThreat(move, 1);
            int combined_threat = ai_threat + human_threat;

            if (combined_threat >= 1000)
            {
                threat_moves.emplace_back(move, combined_threat);
            }
        }

        if (!threat_moves.empty())
        {
            sort(threat_moves.begin(), threat_moves.end(),
                 [](const pair<int, int> &a, const pair<int, int> &b)
                 {
                     return a.second > b.second;
                 });

            best_move = threat_moves[0].first;
            best_score = threat_moves[0].second + 10000;
            found_final = true;
            cout << "DEBUG: Found threat move=" << best_move << endl;
        }
    }

    if (!found_final)
    {
        static thread_local vector<pair<int, int>> move_scores;
        move_scores.clear();
        move_scores.reserve(_active_square_list.size());

        const int alpha_start = LOSS_SCORE;
        const int beta_start = WIN_SCORE;
        int alpha = alpha_start;

        for (int move : _active_square_list)
        {
            if (move < 0 || move >= static_cast<int>(_board.size()) || _board[move] != 0)
                continue;

            applyMove(move, 2);
            const int score = minimax(depth, false, alpha, beta_start);
            undoMove(move, 0);

            move_scores.emplace_back(move, score);

            if (score > best_score)
            {
                best_score = score;
                alpha = max(alpha, score);
            }

            if (score >= WIN_SCORE - depth)
            {
                break;
            }
        }

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

        if (best_moves.size() > 1)
        {
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

                if (ai_last_move != -1)
                {
                    const int ai_row = ai_last_move / _board_size;
                    const int ai_col = ai_last_move % _board_size;
                    const int move_row = move / _board_size;
                    const int move_col = move % _board_size;
                    const int distance = abs(ai_row - move_row) + abs(ai_col - move_col);
                    strategic_score += max(0, 10 - distance);
                }

                const int ai_threat = evaluateMoveThreat(move, 2);
                const int human_threat = evaluateMoveThreat(move, 1);
                strategic_score += ai_threat + (human_threat >> 1);

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

    if (best_move != -1)
    {
        applyMove(best_move, 2);
    }

    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    cout << "AI took " << duration.count() << " ms to decide on move: " << best_move << endl;

    return best_move;
}

int Game::minimax(int depth, bool maximizing_player, int alpha, int beta)
{
    const auto transposition_table_iterator = _transposition_table.find(_zobrist_hash);
    if (transposition_table_iterator != _transposition_table.end())
        return transposition_table_iterator->second;

    if (depth == 0)
        return evalBoard(maximizing_player);

    if (_active_square_list.empty())
        return TIE_SCORE;

    static thread_local vector<int> ordered_moves;
    ordered_moves.clear();

    for (int move : _active_square_list)
    {
        if (move >= 0 && move < static_cast<int>(_board.size()) && _board[move] == 0)
        {
            ordered_moves.push_back(move);
        }
    }

    if (ordered_moves.empty())
        return TIE_SCORE;
    orderMoves(ordered_moves, maximizing_player);

    int best_score = maximizing_player ? INT_MIN : INT_MAX;
    const uint_fast8_t current_player = maximizing_player ? 2 : 1;

    for (int move : ordered_moves)
    {
        applyMove(move, current_player);

        const int check_result = checkBoard(move, current_player);
        int score;

        if (check_result == 2)
        {
            score = TIE_SCORE;
        }
        else if (check_result == 1)
        {
            score = (current_player == 2) ? WIN_SCORE + depth : LOSS_SCORE - depth;
        }
        else
        {
            score = minimax(depth - 1, !maximizing_player, alpha, beta);
        }

        undoMove(move, 0);

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
            break;
        }
    }

    if (depth >= _hash_threshold)
    {
        _transposition_table[_zobrist_hash] = best_score;
    }

    return best_score;
}

int Game::evalBoard(const bool maximizing_player) const
{
    static constexpr int AI_PLAYER = 2;
    static constexpr int HUMAN_PLAYER = 1;

    int score = 0;

    _threat_cache.clear();

    if (_board_size >= 15)
    {
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

            if (ai_threat >= 1000)
                score += ai_threat;
            if (human_threat >= 1000)
                score -= human_threat;
        }
    }
    else if (_board_size >= 8)
    {
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

            if (ai_threat >= 100)
                score += ai_threat;
            if (human_threat >= 100)
                score -= human_threat;
        }
    }
    else
    {
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

int Game::evaluateMoveThreat(int move, uint_fast8_t player) const
{
    if (move < 0 || move >= static_cast<int>(_board.size()))
        return 0;

    if (_board[move] != 0)
        return 0;

    const int win_requirement = _win_count;
    int max_threat_level = 0;

    const auto &direction_info = _direction_data[move];

    for (int dir = 0; dir < DIRECTION_COUNT; dir++)
    {
        const auto &dir_data = direction_info[dir];
        int count = 1;

        const int max_steps_positive = min(win_requirement, dir_data.positive_limit);
        const int max_steps_negative = min(win_requirement, dir_data.negative_limit);

        for (int step = 1; step < max_steps_positive; step++)
        {
            if (_board[move + step * dir_data.step_offset] == player)
                count++;
            else
                break;
        }

        for (int step = 1; step < max_steps_negative; step++)
        {
            if (_board[move - step * dir_data.step_offset] == player)
                count++;
            else
                break;
        }

        if (count >= win_requirement)
        {
            return 10000;
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

void Game::orderMoves(vector<int> &moves, bool maximizing_player) const
{
    const uint_fast8_t player = maximizing_player ? 2 : 1;
    const uint_fast8_t opponent = maximizing_player ? 1 : 2;

    sort(moves.begin(), moves.end(), [this, player, opponent](int a, int b)
         {
        if (a < 0 || a >= static_cast<int>(_board.size()) || 
            b < 0 || b >= static_cast<int>(_board.size()))
            return false;
        
        if (a >= static_cast<int>(_center_distances.size()) || 
            b >= static_cast<int>(_center_distances.size()))
            return false;
        
        int a_threat = evaluateMoveThreat(a, player) + evaluateMoveThreat(a, opponent);
        int b_threat = evaluateMoveThreat(b, player) + evaluateMoveThreat(b, opponent);
        
        if (a_threat == b_threat) {
            return _center_distances[a] < _center_distances[b];
        }
        
        return a_threat > b_threat; });
}