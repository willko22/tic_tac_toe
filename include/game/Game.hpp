#pragma once

class Game {
public:
    Game();
    ~Game();
    
    void run();
    
private:
    void initialize();
    void update();
    void render();
    void cleanup();
    
    bool m_running;
};
