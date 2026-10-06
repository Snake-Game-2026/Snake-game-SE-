// US-07: Display Live Score (FR-7)
// Renderer implementation: live score is drawn every frame from GameSnapshot.
#include "Renderer.h"
#include "GameEngine.h"
#include <string>

void Renderer::drawScore(sf::RenderWindow& window, const GameSnapshot& snapshot) {
    drawText(window, "SCORE", 14, panelLeft_ + 18.0f, 62.0f, sf::Color(150, 165, 185));
    drawText(window, std::to_string(snapshot.score), 34, panelLeft_ + 18.0f, 82.0f,
             sf::Color(245, 248, 255), true);
}

const GameSnapshot& GameEngine::getState() const {
    return state_;
}
