#include "game/Game.hpp"
#include <iostream>

Game::Game() : m_running(false) {
    initialize();
}

Game::~Game() {
    cleanup();
}

void Game::run() {
    m_running = true;
    
    std::cout << "Tower Defense Game Starting..." << std::endl;
    
    while (m_running) {
        update();
        render();
        
        // Simple exit condition for now
        m_running = false;
    }
    
    std::cout << "Game Ended." << std::endl;
}

void Game::initialize() {
    std::cout << "Initializing game..." << std::endl;
    // Initialize SDL, SFML, or other libraries here
}

void Game::update() {
    // Game logic updates
}

void Game::render() {
    // Rendering code
}

void Game::cleanup() {
    std::cout << "Cleaning up..." << std::endl;
    // Cleanup resources
}
