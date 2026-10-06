// US-04: Directional Keyboard Control (FR-4, SO-2)
// Arrow keys and W/A/S/D are mapped. Reverse direction is rejected.
#include "GameController.h"
#include "InputHandler.h"

static bool isDirectionKey(sf::Keyboard::Key key) {
    return key == sf::Keyboard::Up || key == sf::Keyboard::Down ||
           key == sf::Keyboard::Left || key == sf::Keyboard::Right ||
           key == sf::Keyboard::W || key == sf::Keyboard::A ||
           key == sf::Keyboard::S || key == sf::Keyboard::D;
}

static Direction toDirection(sf::Keyboard::Key key) {
    if (key == sf::Keyboard::Up || key == sf::Keyboard::W) return Direction::UP;
    if (key == sf::Keyboard::Down || key == sf::Keyboard::S) return Direction::DOWN;
    if (key == sf::Keyboard::Left || key == sf::Keyboard::A) return Direction::LEFT;
    return Direction::RIGHT;
}

InputHandler::InputHandler(GameController& controller) : controller_(controller) {}

void InputHandler::processEvent(const sf::Event& event) {
    if (event.type != sf::Event::KeyPressed) return;

    const sf::Keyboard::Key key = event.key.code;

    if (key == sf::Keyboard::P) {
        controller_.togglePause();
        return;
    }

    if (key == sf::Keyboard::R) {
        controller_.requestRestart();
        return;
    }

    if (key == sf::Keyboard::Escape || key == sf::Keyboard::Q) {
        controller_.requestExit();
        return;
    }

    if (isDirectionKey(key)) {
        controller_.submitDirection(toDirection(key));
    }
    // All unmapped keys are silently ignored (NFR-3 / SO-2).
}
