// US-05: Snake Movement and Game Tick (FR-5)
// Default tick interval is 150 ms; implementation supports 80-300 ms by code.

#include "GameController.h"
#include <chrono>

static Position advance(Position head, Direction direction) {
    switch (direction) {
        case Direction::UP:
            --head.row;
            break;

        case Direction::DOWN:
            ++head.row;
            break;

        case Direction::LEFT:
            --head.col;
            break;

        case Direction::RIGHT:
            ++head.col;
            break;
    }

    return head;
}

// US-05: Set the snake's movement direction.
// Direct reversal is rejected so the snake cannot immediately
// move into its own body.
bool GameEngine::setHeading(Direction direction) {

    // Reject UP -> DOWN
    if (heading_ == Direction::UP &&
        direction == Direction::DOWN) {
        return false;
    }

    // Reject DOWN -> UP
    if (heading_ == Direction::DOWN &&
        direction == Direction::UP) {
        return false;
    }

    // Reject LEFT -> RIGHT
    if (heading_ == Direction::LEFT &&
        direction == Direction::RIGHT) {
        return false;
    }

    // Reject RIGHT -> LEFT
    if (heading_ == Direction::RIGHT &&
        direction == Direction::LEFT) {
        return false;
    }

    // Valid direction
    heading_ = direction;
    return true;
}

bool GameEngine::isInBounds(Position position) const {
    return position.col >= 0 &&
           position.col < state_.cols &&
           position.row >= 0 &&
           position.row < state_.rows;
}

Position GameEngine::nextHead() const {
    return advance(state_.snake.front(), heading_);
}

TickResult GameEngine::tick() {

    TickResult result;

    // Do not move unless the game is running.
    if (state_.sessionState != SessionState::RUNNING ||
        state_.snake.empty()) {
        return result;
    }

    // Calculate the next head position.
    const Position newHead = nextHead();

    // Check wall/self collision.
    if (detectCollision(newHead)) {
        result.collision = true;
        state_.sessionState = SessionState::GAME_OVER;
        return result;
    }

    // Check whether the snake is eating food.
    const bool eating = (newHead == state_.food);

    // Move the head.
    state_.snake.push_front(newHead);

    // Store the direction used for this movement.
    lastMoveDirection_ = heading_;

    if (eating) {

        // Food consumed:
        // - increase score
        // - increase snake length
        result.ateFood = consumeFood(newHead);

        // Spawn new food.
        if (!spawnFood()) {
            // Board is full.
            state_.sessionState = SessionState::GAME_OVER;
            result.collision = true;
        }

    } else {

        // Normal movement:
        // remove the last segment so the snake
        // remains the same length.
        state_.snake.pop_back();
    }

    return result;
}


// ------------------------------------------------------------
// GameController
// ------------------------------------------------------------

GameController::GameController(GameEngine& engine)
    : engine_(engine) {
}


// Initialize a new game session.
void GameController::initialize() {

    // Load the saved high score.
    engine_.loadHighScore();

    // Reset snake, food, score and game state.
    engine_.reset();

    // Start the session in RUNNING state.
    state_ = SessionState::RUNNING;

    // Reset the movement timer.
    tickClock_.restart();

    // No exit requested.
    exitRequested_ = false;
}


// Receive a direction from the input handler.
void GameController::submitDirection(Direction direction) {

    // Ignore input when the game is not running.
    if (state_ != SessionState::RUNNING) {
        return;
    }

    // GameEngine handles reversal protection.
    engine_.setHeading(direction);
}


// Update the game according to the tick interval.
void GameController::update() {

    // Paused and Game Over states do not move the snake.
    if (state_ != SessionState::RUNNING) {
        return;
    }

    // Wait until the configured tick interval has elapsed.
    if (tickClock_.getElapsedTime().asMilliseconds()
        < tickIntervalMs_) {
        return;
    }

    // Restart the timer for the next movement.
    tickClock_.restart();

    // Move the snake one cell.
    const TickResult result = engine_.tick();

    // Handle collision/Game Over.
    if (result.collision) {

        state_ = SessionState::GAME_OVER;

        // Update and persist high score if required.
        engine_.finalizeSession();
    }
}


// Check whether the user requested application exit.
bool GameController::isExitRequested() const {
    return exitRequested_;
}


// Return the current game state to the renderer.
GameSnapshot GameController::getSnapshot() const {

    GameSnapshot snapshot = engine_.getState();

    // Controller owns the current session state.
    snapshot.sessionState = state_;

    return snapshot;
}