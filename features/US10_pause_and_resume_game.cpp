// US-10 - Pause and Resume Game (FR-10)  [GitHub #44, In Progress]
// Sprint 2 - Snake Game (PES University, UE23CS341A). SFML 3.x, C++17
// Build: g++ -std=c++17 US10_pause_and_resume_game.cpp -o US10_pause_and_resume_game -lsfml-graphics -lsfml-window -lsfml-system
// Run:   ./US10_pause_and_resume_game              (self-test runs first, then the demo window)
//        ./US10_pause_and_resume_game --test       (self-test only, no window)
//        ./US10_pause_and_resume_game --shot x.png (save one frame as a screenshot and exit)
// Contains US-01 .. US-09 (sprint 1) and US-10, so it compiles on its own.

#ifdef NDEBUG
#undef NDEBUG            // the self-tests use assert(); keep it active in every build type
#endif
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cassert>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>


struct Position {
    int col = 0, row = 0;
    bool operator==(const Position& o) const { return col == o.col && row == o.row; }
};
enum class Direction { UP, DOWN, LEFT, RIGHT };

// ===================== shared UI helpers (same look in every file, SFML 3) =====================
namespace ui {
constexpr int   COLS = 20, ROWS = 20, CELL = 24;
constexpr int   WIN_W = 640, WIN_H = 620;
constexpr float BOARD_X = (WIN_W - COLS * CELL) / 2.f;
constexpr float BOARD_Y = 110.f;

const sf::Color BG(13, 13, 18),        BOARD_BG(8, 8, 11);
const sf::Color GRID_LINE(40, 40, 52), BORDER(78, 78, 98);
const sf::Color CARD(22, 22, 30),      CARD_BORDER(52, 52, 68);
const sf::Color OUTLINE(0, 0, 0);
const sf::Color GREEN(106, 190, 48),   LIGHT(153, 229, 80);
const sf::Color TEAL(55, 148, 110),    OLIVE(75, 105, 47);
const sf::Color EYE(251, 242, 54),     TONGUE(172, 50, 50);
const sf::Color MUTED(140, 140, 158),  GOLD(255, 215, 0);
const sf::Color DANGER(230, 70, 70);

inline void rect(sf::RenderTarget& t, const sf::RenderStates& st, float x, float y,
                 float w, float h, sf::Color c) {
    sf::RectangleShape r({w, h});
    r.setPosition({x, y});
    r.setFillColor(c);
    t.draw(r, st);
}
inline std::string argValue(int argc, char** argv, const std::string& key) {
    for (int i = 1; i + 1 < argc; ++i) if (key == argv[i]) return argv[i + 1];
    return "";
}
inline bool hasFlag(int argc, char** argv, const std::string& key) {
    for (int i = 1; i < argc; ++i) if (key == argv[i]) return true;
    return false;
}
// --shot <file.png>: save one frame and quit (used for screenshots in reports)
inline void saveShot(sf::RenderWindow& w, const std::string& path) {
    sf::Texture tex(w.getSize());
    tex.update(w);
    (void)tex.copyToImage().saveToFile(path);
}
}  // namespace ui

// ---------- board drawing (US-01 look) ----------
namespace ui {
inline void drawBoard(sf::RenderTarget& t) {
    sf::RenderStates id;
    rect(t, id, BOARD_X - 3, BOARD_Y - 3, COLS * CELL + 6, ROWS * CELL + 6, BORDER);
    rect(t, id, BOARD_X - 1, BOARD_Y - 1, COLS * CELL + 2, ROWS * CELL + 2, BOARD_BG);
    for (int i = 0; i <= COLS; ++i)
        rect(t, id, BOARD_X + i * CELL - (i == COLS ? 1 : 0), BOARD_Y, 1, ROWS * CELL, GRID_LINE);
    for (int j = 0; j <= ROWS; ++j)
        rect(t, id, BOARD_X, BOARD_Y + j * CELL - (j == ROWS ? 1 : 0), COLS * CELL, 1, GRID_LINE);
}
}  // namespace ui

// ---------- snake drawing (US-02 look) ----------
namespace ui {
inline void drawBodySegment(sf::RenderTarget& t, float x, float y, int idx, bool tail) {
    sf::RenderStates id;
    const float inset = tail ? 3.f : 1.f;
    const float s = CELL - 2 * inset;
    rect(t, id, x + inset, y + inset, s, s, OUTLINE);
    rect(t, id, x + inset + 2, y + inset + 2, s - 4, s - 4, GREEN);
    const float ix = x + inset + 2, iy = y + inset + 2, iw = s - 4, ih = s - 4;
    switch (idx % 3) {
        case 0: rect(t, id, ix, iy, iw, 4, LIGHT); break;
        case 1: rect(t, id, ix, iy + ih / 2, iw, ih / 2, TEAL); break;
        case 2: rect(t, id, ix + iw - 6, iy + ih - 6, 5, 5, OLIVE); break;
    }
}
inline float angleOf(Direction d) {
    switch (d) { case Direction::RIGHT: return 0.f;   case Direction::DOWN: return 90.f;
                 case Direction::LEFT:  return 180.f; default:               return 270.f; }
}
inline void drawHead(sf::RenderTarget& t, float x, float y, Direction dir) {
    sf::Transform tr;
    tr.translate({x + CELL / 2.f, y + CELL / 2.f});
    tr.rotate(sf::degrees(angleOf(dir)));
    tr.translate({-CELL / 2.f, -CELL / 2.f});
    sf::RenderStates st(tr);                      // local coords: facing right
    rect(t, st, CELL, CELL / 2 - 1, 5, 2, TONGUE);
    rect(t, st, CELL + 5, CELL / 2 - 3, 3, 2, TONGUE);
    rect(t, st, CELL + 5, CELL / 2 + 1, 3, 2, TONGUE);
    rect(t, st, 0, 0, CELL, CELL, OUTLINE);
    rect(t, st, 2, 2, CELL - 4, CELL - 4, GREEN);
    rect(t, st, 2, 2, CELL - 4, 4, LIGHT);
    rect(t, st, 2, CELL - 6, CELL - 4, 4, TEAL);
    rect(t, st, 12, 5, 6, 6, EYE);
    rect(t, st, 12, CELL - 11, 6, 6, EYE);
    rect(t, st, 15, 7, 3, 3, OUTLINE);
    rect(t, st, 15, CELL - 9, 3, 3, OUTLINE);
}
inline void drawSnake(sf::RenderTarget& t, const std::deque<Position>& body, Direction dir) {
    for (int i = (int)body.size() - 1; i >= 1; --i)
        drawBodySegment(t, BOARD_X + body[i].col * CELL, BOARD_Y + body[i].row * CELL,
                        i, i == (int)body.size() - 1);
    if (!body.empty()) drawHead(t, BOARD_X + body[0].col * CELL, BOARD_Y + body[0].row * CELL, dir);
}
}  // namespace ui

// ---------- food drawing: pixel-art apple (US-03 look) ----------
// 12 x 14 pixel apple, built in code so no image file is needed at run time.
namespace ui {
inline const sf::Texture& appleTexture() {
    static const char* ART[14] = {
        ".......a....",
        "......ab....",
        "......a.....",
        "...ccabaa...",
        ".ccdcbcecaa.",
        "ccefdddfgcaa",
        "cceeegegeeca",
        "ccegegeeeeeh",
        "acceecceceeh",
        "accccceccech",
        ".accecccech.",
        ".acccceecch.",
        "..hcccccch..",
        "...haaahh...",
    };
    static sf::Texture tex = [] {
        auto col = [](char k) -> sf::Color {
            switch (k) {
                case 'a': return {127, 44, 3};     // stem / rim brown
                case 'b': return {92, 42, 7};      // dark stem
                case 'c': return {165, 14, 23};    // body red
                case 'd': return {249, 159, 166};  // highlight
                case 'e': return {226, 23, 36};    // bright red
                case 'f': return {251, 105, 112};  // pink shine
                case 'g': return {252, 31, 49};    // light red
                case 'h': return {94, 5, 11};      // shadow
                default:  return sf::Color::Transparent;
            }
        };
        sf::Image img({12u, 14u}, sf::Color::Transparent);
        for (unsigned r = 0; r < 14; ++r)
            for (unsigned c = 0; c < 12; ++c) img.setPixel({c, r}, col(ART[r][c]));
        sf::Texture t;
        (void)t.loadFromImage(img);
        t.setSmooth(false);                // keep crisp pixel edges
        return t;
    }();
    return tex;
}
inline void drawApple(sf::RenderTarget& t, Position p) {
    sf::Sprite s(appleTexture());
    s.setScale({1.5f, 1.5f});              // 18 x 21 px inside a 24 px cell
    s.setPosition({BOARD_X + p.col * CELL + 3.f, BOARD_Y + p.row * CELL + 1.f});
    t.draw(s);
}
}  // namespace ui

// ---------- HUD (US-07 look) ----------
namespace ui {
struct Hud {
    sf::Font font;
    // Portable font search (NFR-6): a bundled font first, then the usual system fonts on
    // Windows, Linux and macOS. Each path is checked first so SFML prints no errors for misses.
    bool load() {
        std::vector<std::string> paths = {"assets/font.ttf", "font.ttf"};
        if (const char* win = std::getenv("WINDIR")) {
            paths.push_back(std::string(win) + "/Fonts/segoeui.ttf");
            paths.push_back(std::string(win) + "/Fonts/arial.ttf");
        }
        for (const char* p : {"C:/Windows/Fonts/segoeui.ttf", "C:/Windows/Fonts/arial.ttf",
                              "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                              "/usr/share/fonts/TTF/DejaVuSans.ttf",
                              "/usr/share/fonts/dejavu/DejaVuSans.ttf",
                              "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
                              "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
                              "/System/Library/Fonts/Supplemental/Arial.ttf",
                              "/Library/Fonts/Arial.ttf"})
            paths.push_back(p);
        for (const auto& f : paths) {
            std::error_code ec;
            if (std::filesystem::exists(f, ec) && font.openFromFile(f)) return true;
        }
        return false;
    }
    // Width of a piece of text in pixels (used to line up columns).
    float width(const std::string& s, unsigned size, bool bold = false) const {
        sf::Text tx(font, s, size);
        if (bold) tx.setStyle(sf::Text::Bold);
        return tx.getLocalBounds().size.x;
    }
    // Small pill at the top of the board, e.g. "NEW GAME".
    void banner(sf::RenderTarget& t, const std::string& s, sf::Color fill, sf::Color ink) {
        sf::RenderStates id;
        const float w = width(s, 13, true) + 28;
        rect(t, id, WIN_W / 2.f - w / 2, BOARD_Y + 12, w, 26, fill);
        centered(t, s, 13, ink, BOARD_Y + 16, true);
    }
    void text(sf::RenderTarget& t, const std::string& s, unsigned size, sf::Color c,
              float x, float y, bool bold = false) {
        sf::Text tx(font, s, size);
        tx.setFillColor(c);
        if (bold) tx.setStyle(sf::Text::Bold);
        tx.setPosition({x, y});
        t.draw(tx);
    }
    void centered(sf::RenderTarget& t, const std::string& s, unsigned size, sf::Color c,
                  float y, bool bold = false) {
        sf::Text tx(font, s, size);
        tx.setFillColor(c);
        if (bold) tx.setStyle(sf::Text::Bold);
        tx.setPosition({WIN_W / 2.f - tx.getLocalBounds().size.x / 2.f, y});
        t.draw(tx);
    }
    void card(sf::RenderTarget& t, float x, const std::string& label, int value, sf::Color c) {
        sf::RenderStates id;
        rect(t, id, x, 24, 115, 62, CARD_BORDER);
        rect(t, id, x + 1, 25, 113, 60, CARD);
        text(t, label, 11, MUTED, x + 14, 32);
        text(t, std::to_string(value), 28, c, x + 14, 46, true);
    }
    // Title + subtitle on the left, SCORE and BEST cards on the right.
    void draw(sf::RenderTarget& t, int score, int best, const std::string& subtitle) {
        text(t, "SNAKE", 34, GREEN, BOARD_X, 28, true);
        text(t, subtitle, 12, MUTED, BOARD_X + 2, 70);
        card(t, 320, "SCORE", score, sf::Color::White);
        card(t, 445, "BEST", best, GOLD);
    }
    // Muted key-hint line under the board.
    void hint(sf::RenderTarget& t, const std::string& s) { centered(t, s, 12, MUTED, 596); }
    // Dimmed panel over the board with a title and a line of detail.
    void overlay(sf::RenderTarget& t, const std::string& title, const std::string& sub) {
        sf::RenderStates id;
        rect(t, id, BOARD_X, BOARD_Y, COLS * CELL, ROWS * CELL, sf::Color(0, 0, 0, 175));
        centered(t, title, 38, sf::Color::White, 220, true);
        centered(t, sub, 15, MUTED, 275);
    }
};
}  // namespace ui

// Creates the standard 640x620 game window.
inline sf::RenderWindow makeWindow(const std::string& title) {
    sf::RenderWindow w(sf::VideoMode({(unsigned)ui::WIN_W, (unsigned)ui::WIN_H}), title,
                       sf::Style::Titlebar | sf::Style::Close);
    w.setFramerateLimit(60);
    return w;
}

// ===================== US-01: Initialize Game Grid (FR-1, SEC-4) =====================
// Rectangular grid, configurable size, default 20 columns x 20 rows.
class GameGrid {
public:
    static constexpr int DEFAULT_COLS = 20, DEFAULT_ROWS = 20, MIN_SIZE = 5;

    explicit GameGrid(int cols = DEFAULT_COLS, int rows = DEFAULT_ROWS) {
        if (cols < MIN_SIZE || rows < MIN_SIZE) { cols = DEFAULT_COLS; rows = DEFAULT_ROWS; }
        cols_ = cols; rows_ = rows;
        cells_.assign(static_cast<size_t>(cols_) * rows_, 0);
    }
    int cols() const { return cols_; }
    int rows() const { return rows_; }

    // SEC-4: every access goes through this check
    bool inBounds(Position p) const {
        return p.col >= 0 && p.col < cols_ && p.row >= 0 && p.row < rows_;
    }
    bool isOccupied(Position p) const { return inBounds(p) && cells_[idx(p)] != 0; }
    bool setOccupied(Position p, bool v) {
        if (!inBounds(p)) return false;
        cells_[idx(p)] = v ? 1 : 0;
        return true;
    }
    void clear() { cells_.assign(cells_.size(), 0); }

private:
    int cols_, rows_;
    std::vector<std::uint8_t> cells_;
    int idx(Position p) const { return p.row * cols_ + p.col; }
};

// ===================== US-02: Initialize Snake (FR-2) =====================
// Centre of grid, length 3, heading RIGHT. Body stored head-first.
class Snake {
public:
    static constexpr size_t INITIAL_LENGTH = 3;

    void init(int gridCols, int gridRows) {
        body_.clear();
        heading_ = pending_ = Direction::RIGHT;
        const int c = gridCols / 2, r = gridRows / 2;
        for (size_t i = 0; i < INITIAL_LENGTH; ++i)
            body_.push_back({c - static_cast<int>(i), r});
    }
    const std::deque<Position>& body() const { return body_; }
    Position head() const { return body_.front(); }
    Position tail() const { return body_.back(); }
    Direction heading() const { return heading_; }   // direction of the last move
    Direction pending() const { return pending_; }   // direction for the next move
    size_t length() const { return body_.size(); }

    // Low-level edits used by the movement / control stories.
    void setPending(Direction d) { pending_ = d; }
    void commitHeading() { heading_ = pending_; }
    void pushHead(Position p) { body_.push_front(p); }
    void popTail() { if (!body_.empty()) body_.pop_back(); }

private:
    std::deque<Position> body_;
    Direction heading_ = Direction::RIGHT, pending_ = Direction::RIGHT;
};

// Marks the snake's cells as occupied on the grid (used by food spawning & collisions).
inline void syncGrid(GameGrid& g, const Snake& s) {
    g.clear();
    for (const auto& p : s.body()) g.setOccupied(p, true);
}

// ===================== US-03: Spawn Food (FR-3) =====================
// Exactly one food item at a random UNOCCUPIED cell, at game start and after each meal.
class FoodSpawner {
public:
    explicit FoodSpawner(unsigned seed = std::random_device{}()) : rng_(seed) {}

    // Returns the new food cell, or nullopt if the board is completely full.
    std::optional<Position> spawn(const GameGrid& g) {
        std::vector<Position> freeCells;
        freeCells.reserve(static_cast<size_t>(g.cols()) * g.rows());
        for (int r = 0; r < g.rows(); ++r)
            for (int c = 0; c < g.cols(); ++c)
                if (!g.isOccupied({c, r})) freeCells.push_back({c, r});
        if (freeCells.empty()) return std::nullopt;
        std::uniform_int_distribution<size_t> pick(0, freeCells.size() - 1);
        return freeCells[pick(rng_)];
    }

private:
    std::mt19937 rng_;
};

// ===================== US-04: Directional Keyboard Control (FR-4, NFR-3, SO-2) =====================
// Arrow keys or W/A/S/D change the heading. A direct reversal (e.g. RIGHT -> LEFT) is ignored.
inline Direction opposite(Direction d) {
    switch (d) { case Direction::UP: return Direction::DOWN; case Direction::DOWN: return Direction::UP;
                 case Direction::LEFT: return Direction::RIGHT; default: return Direction::LEFT; }
}
inline bool isReversal(Direction current, Direction requested) { return requested == opposite(current); }

// Maps a key to a direction; any other key returns nullopt and is simply ignored.
inline std::optional<Direction> directionForKey(sf::Keyboard::Key k) {
    using K = sf::Keyboard::Key;
    switch (k) {
        case K::Up:    case K::W: return Direction::UP;
        case K::Down:  case K::S: return Direction::DOWN;
        case K::Left:  case K::A: return Direction::LEFT;
        case K::Right: case K::D: return Direction::RIGHT;
        default: return std::nullopt;
    }
}

// Checked against the heading of the LAST MOVE (not the pending one), so two quick key
// presses inside one tick (moving RIGHT, press UP then LEFT) can never fold the snake onto itself.
inline bool requestDirection(Snake& s, Direction d) {
    if (isReversal(s.heading(), d)) return false;
    s.setPending(d);
    return true;
}
inline const char* dirName(Direction d) {
    switch (d) { case Direction::UP: return "UP"; case Direction::DOWN: return "DOWN";
                 case Direction::LEFT: return "LEFT"; default: return "RIGHT"; }
}

// ===================== US-05: Snake Movement and Game Tick (FR-5, NFR-2) =====================
// Every tick the snake moves exactly one cell. Default tick 150 ms, allowed 80..300 ms.
inline Position stepFrom(Position p, Direction d) {
    switch (d) { case Direction::UP: --p.row; break; case Direction::DOWN: ++p.row; break;
                 case Direction::LEFT: --p.col; break; default: ++p.col; }
    return p;
}
inline Position nextHead(const Snake& s) { return stepFrom(s.head(), s.pending()); }

// Moves one cell. If grow == false the tail is removed, so the length stays the same.
inline void advance(Snake& s, bool grow) {
    s.pushHead(nextHead(s));
    s.commitHeading();
    if (!grow) s.popTail();
}

class TickTimer {
public:
    static constexpr int DEFAULT_MS = 150, MIN_MS = 80, MAX_MS = 300;
    explicit TickTimer(int ms = DEFAULT_MS) { setInterval(ms); }

    void setInterval(int ms) { interval_ = ms < MIN_MS ? MIN_MS : (ms > MAX_MS ? MAX_MS : ms); }
    int intervalMs() const { return interval_; }
    void reset() { acc_ = 0; }

    // Feed the frame time; returns true when a tick is due (at most one tick per call).
    bool update(int elapsedMs) {
        acc_ += elapsedMs;
        if (acc_ > 3 * interval_) acc_ = interval_;        // after a stall, don't burst-move
        if (acc_ < interval_) return false;
        acc_ -= interval_;
        return true;
    }

private:
    int interval_ = DEFAULT_MS, acc_ = 0;
};

// ===================== US-06: Food Consumption and Growth (FR-6) =====================
// Head reaches the food -> snake grows by 1 segment and score +10, in the same tick.
constexpr int POINTS_PER_FOOD = 10;

class Score {
public:
    void reset() { value_ = 0; }
    void add(int pts) { value_ += pts; }
    int value() const { return value_; }
private:
    int value_ = 0;
};

// Performs one move. Returns true if the food was eaten during this move.
inline bool moveAndEat(Snake& s, Position food, Score& score) {
    const bool eat = (nextHead(s) == food);
    advance(s, eat);                       // grow = keep the tail this tick
    if (eat) score.add(POINTS_PER_FOOD);
    return eat;
}

// ===================== US-07: Display Live Score (FR-7, FR-15) =====================
// The HUD shows SCORE and BEST. The shown value is refreshed in the same tick it changes.
class ScoreDisplay {
public:
    // Call once per tick after the game logic; returns true if the shown value changed.
    bool update(int score) {
        const bool changed = (score != shown_);
        shown_ = score;
        return changed;
    }
    int shown() const { return shown_; }
    void draw(sf::RenderTarget& t, ui::Hud& hud, int best, const std::string& subtitle) const {
        hud.draw(t, shown_, best, subtitle);
    }
private:
    int shown_ = 0;
};

// ===================== US-08: Collision Detection (FR-8, SEC-4) =====================
// A collision happens when the NEXT head cell is (a) outside the grid or (b) on the snake's body.
enum class Collision { NONE, WALL, SELF };

// growing = the snake eats on this move, so its tail does NOT move out of the way.
inline Collision checkCollision(const GameGrid& g, const Snake& s, Position nh, bool growing) {
    if (!g.inBounds(nh)) return Collision::WALL;
    const auto& b = s.body();
    const size_t n = growing ? b.size() : b.size() - 1;   // the tail cell is vacated this tick
    for (size_t i = 0; i < n; ++i)
        if (b[i] == nh) return Collision::SELF;
    return Collision::NONE;
}
inline const char* collisionName(Collision c) {
    switch (c) { case Collision::WALL: return "wall"; case Collision::SELF: return "own body";
                 default: return "none"; }
}

// ===================== US-09: Game Over (FR-9) =====================
// On collision: stop movement within the same tick and show "Game Over" with the final score.
enum class GameState { RUNNING, GAME_OVER };

class GameSession {
public:
    void start() { state_ = GameState::RUNNING; finalScore_ = 0; cause_ = Collision::NONE; }
    void end(int finalScore, Collision cause) {
        if (state_ == GameState::GAME_OVER) return;     // end only once
        state_ = GameState::GAME_OVER;
        finalScore_ = finalScore;
        cause_ = cause;
    }
    bool isOver() const { return state_ == GameState::GAME_OVER; }
    bool canMove() const { return state_ == GameState::RUNNING; }
    int finalScore() const { return finalScore_; }
    Collision cause() const { return cause_; }
private:
    GameState state_ = GameState::RUNNING;
    int finalScore_ = 0;
    Collision cause_ = Collision::NONE;
};

// One full game tick: collision check first, then move/eat. Returns true if the game ended.
inline bool gameTick(GameGrid& g, Snake& s, Position& food, FoodSpawner& spawner,
                     Score& score, GameSession& session) {
    if (!session.canMove()) return false;
    const Position nh = nextHead(s);
    const bool eating = (nh == food);
    const Collision c = checkCollision(g, s, nh, eating);
    if (c != Collision::NONE) { session.end(score.value(), c); return true; }
    moveAndEat(s, food, score);
    syncGrid(g, s);
    if (eating) {
        auto f = spawner.spawn(g);
        if (!f) { session.end(score.value(), Collision::NONE); return true; }   // board full: win
        food = *f;
    }
    return false;
}

// Draws the standard frame: HUD, hint line, board, apple and snake (used by every demo window).
inline void drawScene(sf::RenderTarget& t, ui::Hud& hud, const Snake& s, Position food, int score,
                      int best, const std::string& subtitle, const std::string& hint) {
    hud.draw(t, score, best, subtitle);
    hud.hint(t, hint);
    ui::drawBoard(t);
    ui::drawApple(t, food);
    ui::drawSnake(t, s.body(), s.heading());
}

// Sprint-1 regression check (US-01 .. US-09) - run first by every sprint-2 self-test.
static void testSprint1() {
    GameGrid g;                       assert(g.cols() == 20 && g.rows() == 20);
    Snake s; s.init(20, 20);          assert(s.length() == 3 && s.head() == (Position{10, 10}));
    syncGrid(g, s);
    FoodSpawner sp(1);
    for (int i = 0; i < 200; ++i) { auto f = sp.spawn(g); assert(f && !g.isOccupied(*f)); }
    assert(!requestDirection(s, Direction::LEFT));
    TickTimer t;                      assert(t.intervalMs() == 150);
    Score sc;
    assert(moveAndEat(s, nextHead(s), sc) && s.length() == 4 && sc.value() == 10);
    assert(checkCollision(g, s, {20, 10}, false) == Collision::WALL);
    GameSession session;              session.start();
    Position food{0, 0};
    while (!gameTick(g, s, food, sp, sc, session)) {}
    assert(session.isOver() && !session.canMove());
    std::cout << "Sprint 1 regression OK (US-01 .. US-09)\n";
}

// ---------- sprint-2 shared state: everything US-01 .. US-09 creates, kept in one place ----------
struct World {
    GameGrid    grid{ui::COLS, ui::ROWS};
    Snake       snake;
    FoodSpawner spawner;
    Position    food{};
    Score       score;
    GameSession session;
    explicit World(unsigned seed = std::random_device{}()) : spawner(seed) {}
};

// FR-1 .. FR-3: empty grid, snake in the centre (length 3, heading RIGHT), one apple, score 0.
inline void startWorld(World& w) {
    w.snake.init(w.grid.cols(), w.grid.rows());
    syncGrid(w.grid, w.snake);
    w.food = *w.spawner.spawn(w.grid);
    w.score.reset();
    w.session.start();
}
// One game tick on the whole world (US-05 .. US-09). Returns true if the game ended.
inline bool tickWorld(World& w) {
    return gameTick(w.grid, w.snake, w.food, w.spawner, w.score, w.session);
}
// Screenshot / test helper: the snake eats n apples placed straight ahead of it.
inline void demoEat(World& w, int n) {
    for (int i = 0; i < n; ++i) { w.food = nextHead(w.snake); tickWorld(w); }
}

// ===================== US-10: Pause and Resume Game (FR-10) =====================
// P or Space toggles between RUNNING and PAUSED.
//  - While paused no tick runs, so the snake, the apple and the score are frozen.
//  - Direction keys pressed while paused are ignored (no free turn while the game is stopped).
//  - On resume the tick timer starts a fresh 150 ms interval, so the snake never jumps.
//  - Losing window focus (alt-tab) also pauses, so the snake does not die unattended.
//  - Pausing after Game Over does nothing.
inline bool isPauseKey(sf::Keyboard::Key k) {
    return k == sf::Keyboard::Key::P || k == sf::Keyboard::Key::Space;
}

class PauseControl {
public:
    // Returns true if the state changed.
    bool toggle(const GameSession& s, TickTimer& t) {
        if (s.isOver()) return false;
        paused_ = !paused_;
        t.reset();                                   // resume = fresh interval, no jump
        return true;
    }
    // Pause only (used when the window loses focus). Returns true if it paused now.
    bool pause(const GameSession& s, TickTimer& t) { return !paused_ && toggle(s, t); }
    void reset() { paused_ = false; }
    bool paused() const { return paused_; }
private:
    bool paused_ = false;
};

// One frame of game logic: a tick runs only while RUNNING. Returns true if the game ended.
inline bool updateWorld(World& w, TickTimer& t, const PauseControl& p, int elapsedMs) {
    if (p.paused() || !w.session.canMove()) return false;
    if (!t.update(elapsedMs)) return false;
    return tickWorld(w);
}
// Steering only counts while RUNNING (FR-10: input is ignored while paused).
inline bool steer(World& w, const PauseControl& p, Direction d) {
    if (p.paused() || !w.session.canMove()) return false;
    return requestDirection(w.snake, d);
}

static void testUS10() {
    World w(3);
    startWorld(w);
    w.food = {0, 0};                                         // keep the apple out of the way
    TickTimer t;
    PauseControl p;
    assert(isPauseKey(sf::Keyboard::Key::P) && isPauseKey(sf::Keyboard::Key::Space));
    assert(!isPauseKey(sf::Keyboard::Key::Q));

    updateWorld(w, t, p, 150);
    assert(w.snake.head() == (Position{11, 10}));
    assert(p.toggle(w.session, t) && p.paused());
    const Position frozen = w.snake.head();
    for (int i = 0; i < 100; ++i) assert(!updateWorld(w, t, p, 50));   // 5 s of paused frames
    assert(w.snake.head() == frozen && w.snake.length() == 3 && w.score.value() == 0);
    assert(!steer(w, p, Direction::UP) && w.snake.pending() == Direction::RIGHT);

    assert(p.toggle(w.session, t) && !p.paused());           // resume
    updateWorld(w, t, p, 149);
    assert(w.snake.head() == frozen);                         // fresh interval: no jump
    updateWorld(w, t, p, 1);
    assert(w.snake.head() == (Position{12, 10}));             // then exactly one cell

    assert(p.pause(w.session, t) && p.paused() && !p.pause(w.session, t));   // focus lost
    p.reset();
    w.session.end(0, Collision::WALL);
    assert(!p.toggle(w.session, t) && !p.paused());           // no pause after Game Over
    std::cout << "US-10 OK: pause freezes the game, input ignored, resume continues one cell per tick\n";
}

// Self-test: sprint-1 regression, then every sprint-2 story up to this one.
static void selfTest() {
    testSprint1();
    testUS10();
}

int main(int argc, char** argv) {
    selfTest();
    if (ui::hasFlag(argc, argv, "--test")) return 0;

    const std::string shot = ui::argValue(argc, argv, "--shot");
    World w(shot.empty() ? std::random_device{}() : 10u);
    startWorld(w);
    TickTimer timer;
    PauseControl pause;
    if (!shot.empty()) {                                      // screenshot: eat 4, turn up, then pause
        demoEat(w, 4);
        requestDirection(w.snake, Direction::UP);
        for (int i = 0; i < 3; ++i) tickWorld(w);
        pause.toggle(w.session, timer);
    }

    auto window = makeWindow("US-10 Pause and Resume");
    ui::Hud hud;
    if (!hud.load()) { std::cerr << "Error: could not load a font\n"; return 1; }
    sf::Clock frame;
    while (window.isOpen()) {
        while (const std::optional ev = window.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) window.close();
            else if (ev->is<sf::Event::FocusLost>()) pause.pause(w.session, timer);
            else if (const auto* k = ev->getIf<sf::Event::KeyPressed>()) {
                if (k->code == sf::Keyboard::Key::Escape) window.close();
                else if (isPauseKey(k->code)) pause.toggle(w.session, timer);
                else if (k->code == sf::Keyboard::Key::R && w.session.isOver()) { startWorld(w); timer.reset(); }
                else if (auto d = directionForKey(k->code)) steer(w, pause, *d);
            }
        }
        updateWorld(w, timer, pause, frame.restart().asMilliseconds());

        window.clear(ui::BG);
        drawScene(window, hud, w.snake, w.food, w.score.value(), 0,
                  w.session.isOver() ? "US-10  game over" : (pause.paused() ? "US-10  paused" : "US-10  running"),
                  "Move: arrows/WASD   P/Space: pause / resume   Esc: quit");
        if (w.session.isOver()) hud.overlay(window, "GAME OVER", "R to play again   -   Esc to quit");
        else if (pause.paused()) hud.overlay(window, "PAUSED", "P / Space to resume   -   snake, apple and score are frozen");
        window.display();
        if (!shot.empty()) { ui::saveShot(window, shot); window.close(); }
    }
}
