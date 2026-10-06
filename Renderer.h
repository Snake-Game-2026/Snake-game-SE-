#pragma once
#include "GameTypes.h"
#include <SFML/Graphics.hpp>
#include <string>

class Renderer {
public:
    Renderer();
    bool loadFont();
    void draw(sf::RenderWindow& window, const GameSnapshot& snapshot);

private:
    void drawGrid(sf::RenderWindow& window, const GameSnapshot& snapshot);
    void drawSnake(sf::RenderWindow& window, const GameSnapshot& snapshot);
    void drawFood(sf::RenderWindow& window, const GameSnapshot& snapshot);
    void drawScore(sf::RenderWindow& window, const GameSnapshot& snapshot);
    void drawHighScore(sf::RenderWindow& window, const GameSnapshot& snapshot);
    void drawControls(sf::RenderWindow& window, const GameSnapshot& snapshot);
    void drawPauseOverlay(sf::RenderWindow& window, const GameSnapshot& snapshot);
    void drawGameOverOverlay(sf::RenderWindow& window, const GameSnapshot& snapshot);
    void drawText(sf::RenderWindow& window, const std::string& text, unsigned size,
                  float x, float y, sf::Color color, bool bold = false);
    bool loadFontFromCandidates();

    sf::Font font_;
    bool fontLoaded_ = false;
    const float cellSize_ = 18.0f;
    const float boardLeft_ = 20.0f;
    const float boardTop_ = 54.0f;
    const float boardSize_ = 360.0f;
    const float panelLeft_ = 400.0f;
};
