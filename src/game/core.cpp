#include "game/Game.hpp"
#include <iostream>

using namespace std;

Game::Game() : _running(false) {}

Game::~Game()
{
    cleanup();
}

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
            uint_fast8_t player = _players_turn ? 1 : 2;

            if (_with_ai && !_players_turn)
            {
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
                loop = false;
            }
            else if (check == 2)
            {
                cout << "It's a tie!" << endl;
                loop = false;
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

        if (!input.empty() && (input[0] == 'n' || input[0] == 'N'))
        {
            _running = false;
        }
        else
        {
            _players_turn = true;
        }
    }
}