#include "Renderer.h"
#include <array>
#include <iostream>

Renderer::Renderer() = default;

bool Renderer::loadFont() {
    fontLoaded_ = loadFontFromCandidates();
    return fontLoaded_;
}

bool Renderer::loadFontFromCandidates() {
#ifdef _WIN32
    const std::array<const char*, 4> paths = {
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/calibri.ttf",
        "C:/Windows/Fonts/tahoma.ttf"
    };
#elif __APPLE__
    const std::array<const char*, 3> paths = {
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/SFNS.ttf",
        "/Library/Fonts/Arial.ttf"
    };
#else
    const std::array<const char*, 4> paths = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
    };
#endif

    for (const char* path : paths) {
        if (font_.loadFromFile(path)) return true;
    }
    return false;
}

void Renderer::drawText(sf::RenderWindow& window, const std::string& text, unsigned size,
                        float x, float y, sf::Color color, bool bold) {
    if (!fontLoaded_) return;
    sf::Text label(text, font_, size);
    label.setPosition(x, y);
    label.setFillColor(color);
    if (bold) label.setStyle(sf::Text::Bold);
    window.draw(label);
}

void Renderer::draw(sf::RenderWindow& window, const GameSnapshot& snapshot) {
    // Header
    sf::RectangleShape header(sf::Vector2f(640.0f, 44.0f));
    header.setPosition(0.0f, 0.0f);
    header.setFillColor(sf::Color(20, 27, 39));
    window.draw(header);
    drawText(window, "SNAKE", 22, 20.0f, 9.0f, sf::Color(120, 235, 160), true);
    drawText(window, "PES UNIVERSITY • SOFTWARE ENGINEERING", 12, 405.0f, 14.0f,
             sf::Color(125, 140, 160));

    drawGrid(window, snapshot);
    drawSnake(window, snapshot);
    drawFood(window, snapshot);
    drawScore(window, snapshot);
    drawHighScore(window, snapshot);
    drawControls(window, snapshot);

    if (snapshot.sessionState == SessionState::PAUSED) {
        drawPauseOverlay(window, snapshot);
    } else if (snapshot.sessionState == SessionState::GAME_OVER) {
        drawGameOverOverlay(window, snapshot);
    }
}

void Renderer::drawGrid(sf::RenderWindow& window, const GameSnapshot& snapshot) {
    sf::RectangleShape board(sf::Vector2f(boardSize_, boardSize_));
    board.setPosition(boardLeft_, boardTop_);
    board.setFillColor(sf::Color(17, 24, 34));
    board.setOutlineThickness(2.0f);
    board.setOutlineColor(sf::Color(53, 67, 87));
    window.draw(board);

    sf::RectangleShape line;
    line.setFillColor(sf::Color(27, 37, 51));

    for (int col = 1; col < snapshot.cols; ++col) {
        line.setSize(sf::Vector2f(1.0f, boardSize_));
        line.setPosition(boardLeft_ + col * cellSize_, boardTop_);
        window.draw(line);
    }

    for (int row = 1; row < snapshot.rows; ++row) {
        line.setSize(sf::Vector2f(boardSize_, 1.0f));
        line.setPosition(boardLeft_, boardTop_ + row * cellSize_);
        window.draw(line);
    }
}

void Renderer::drawSnake(sf::RenderWindow& window, const GameSnapshot& snapshot) {
    for (size_t i = 0; i < snapshot.snake.size(); ++i) {
        const Position& segment = snapshot.snake[i];
        sf::RectangleShape part(sf::Vector2f(cellSize_ - 3.0f, cellSize_ - 3.0f));
        part.setPosition(boardLeft_ + segment.col * cellSize_ + 1.5f,
                         boardTop_ + segment.row * cellSize_ + 1.5f);
        part.setFillColor(i == 0 ? sf::Color(120, 235, 160) : sf::Color(72, 185, 125));
        part.setOutlineThickness(1.0f);
        part.setOutlineColor(sf::Color(35, 80, 60));
        window.draw(part);
    }
}

void Renderer::drawFood(sf::RenderWindow& window, const GameSnapshot& snapshot) {
    sf::RectangleShape food(sf::Vector2f(cellSize_ - 5.0f, cellSize_ - 5.0f));
    food.setPosition(boardLeft_ + snapshot.food.col * cellSize_ + 2.5f,
                     boardTop_ + snapshot.food.row * cellSize_ + 2.5f);
    food.setFillColor(sf::Color(255, 105, 115));
    food.setOutlineThickness(2.0f);
    food.setOutlineColor(sf::Color(255, 170, 175));
    window.draw(food);
}

void Renderer::drawControls(sf::RenderWindow& window, const GameSnapshot&) {
    sf::RectangleShape controls(sf::Vector2f(220.0f, 110.0f));
    controls.setPosition(panelLeft_, 260.0f);
    controls.setFillColor(sf::Color(20, 27, 39));
    controls.setOutlineThickness(1.0f);
    controls.setOutlineColor(sf::Color(48, 62, 80));
    window.draw(controls);

    drawText(window, "CONTROLS", 13, panelLeft_ + 18.0f, 274.0f, sf::Color(150, 165, 185), true);
    drawText(window, "Arrow keys / W A S D", 14, panelLeft_ + 18.0f, 298.0f, sf::Color(225, 232, 242));
    drawText(window, "P  Pause / Resume", 14, panelLeft_ + 18.0f, 320.0f, sf::Color(225, 232, 242));
    drawText(window, "R  Restart after Game Over", 14, panelLeft_ + 18.0f, 342.0f, sf::Color(225, 232, 242));
    drawText(window, "Esc / Q  Exit after Game Over", 14, panelLeft_ + 18.0f, 364.0f, sf::Color(225, 232, 242));

    sf::RectangleShape legend(sf::Vector2f(220.0f, 54.0f));
    legend.setPosition(panelLeft_, 386.0f);
    legend.setFillColor(sf::Color(20, 27, 39));
    window.draw(legend);
    drawText(window, "■  Snake     ■  Food", 13, panelLeft_ + 18.0f, 401.0f, sf::Color(175, 188, 205));
}
