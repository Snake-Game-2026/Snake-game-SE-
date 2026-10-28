// US-14 - Update and Persist High Score (FR-14, SEC-1, SO-1, NFR-4)  [GitHub #48, Todo]
// Sprint 2 - Snake Game (PES University, UE23CS341A). SFML 3.x, C++17
// Build: g++ -std=c++17 US14_update_and_persist_high_score.cpp -o US14_update_and_persist_high_score -lsfml-graphics -lsfml-window -lsfml-system
// Run:   ./US14_update_and_persist_high_score              (self-test runs first, then the demo window)
//        ./US14_update_and_persist_high_score --test       (self-test only, no window)
//        ./US14_update_and_persist_high_score --shot x.png (save one frame as a screenshot and exit)
// Contains US-01 .. US-09 (sprint 1) and US-10 .. US-14, so it compiles on its own.

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

// ===================== US-11: Restart Game (FR-11) =====================
// R starts a brand-new game: grid cleared, snake back in the centre (length 3, heading RIGHT),
// a new apple, score 0, state RUNNING, not paused.
// Allowed after Game Over or while paused, so a stray R in the middle of a run cannot wipe it.
// The high score (BEST) is not part of the world and is never touched by a restart.
inline bool isRestartKey(sf::Keyboard::Key k) { return k == sf::Keyboard::Key::R; }
inline bool canRestart(const GameSession& s, const PauseControl& p) { return s.isOver() || p.paused(); }

// Returns true if the game was restarted.
inline bool restartGame(World& w, TickTimer& t, PauseControl& p) {
    if (!canRestart(w.session, p)) return false;
    startWorld(w);
    t.reset();
    p.reset();
    return true;
}

// True when the world is exactly in the FR-1 .. FR-3 start state.
inline bool isFreshStart(const World& w) {
    const auto& b = w.snake.body();
    const int c = w.grid.cols() / 2, r = w.grid.rows() / 2;
    return b.size() == Snake::INITIAL_LENGTH && b[0] == (Position{c, r}) &&
           b[1] == (Position{c - 1, r}) && b[2] == (Position{c - 2, r}) &&
           w.snake.heading() == Direction::RIGHT && w.snake.pending() == Direction::RIGHT &&
           w.score.value() == 0 && w.session.canMove() &&
           w.grid.inBounds(w.food) && !w.grid.isOccupied(w.food);
}

static void testUS11() {
    World w(11);
    startWorld(w);
    TickTimer t;
    PauseControl p;
    assert(isRestartKey(sf::Keyboard::Key::R) && !isRestartKey(sf::Keyboard::Key::T));

    demoEat(w, 2);
    assert(w.score.value() == 20 && w.snake.length() == 5);
    assert(!restartGame(w, t, p) && w.score.value() == 20);   // running: R is ignored

    p.toggle(w.session, t);                                    // paused: restart allowed
    assert(restartGame(w, t, p) && isFreshStart(w) && !p.paused());

    while (!tickWorld(w)) {}                                   // straight into the wall
    assert(w.session.isOver());
    assert(restartGame(w, t, p) && isFreshStart(w));           // after Game Over

    for (int i = 0; i < 500; ++i) {                            // many restarts: always a clean start
        w.session.end(0, Collision::SELF);
        assert(restartGame(w, t, p) && isFreshStart(w));
    }
    std::cout << "US-11 OK: restart (paused / game over) resets snake, food, score and state\n";
}

// ===================== US-12: Exit Game (FR-12) =====================
// Esc or the window's close button quits from any state (running, paused, game over).
// An exit hook runs exactly once before the window closes - the full game uses it to save
// the high score, so quitting never loses a new best. A second request (Esc, then the close
// button) is ignored.
enum class ExitReason { NONE, ESCAPE_KEY, WINDOW_CLOSED };

inline bool isExitKey(sf::Keyboard::Key k) { return k == sf::Keyboard::Key::Escape; }
inline const char* exitReasonName(ExitReason r) {
    switch (r) { case ExitReason::ESCAPE_KEY: return "Esc key"; case ExitReason::WINDOW_CLOSED: return "window closed";
                 default: return "none"; }
}

class ExitControl {
public:
    void setOnExit(std::function<void()> f) { onExit_ = std::move(f); }
    // First request wins. Returns true if this call started the exit.
    bool request(ExitReason r) {
        if (r == ExitReason::NONE || reason_ != ExitReason::NONE) return false;
        reason_ = r;
        if (onExit_) onExit_();
        return true;
    }
    bool requested() const { return reason_ != ExitReason::NONE; }
    ExitReason reason() const { return reason_; }
private:
    ExitReason reason_ = ExitReason::NONE;
    std::function<void()> onExit_;
};

static void testUS12() {
    assert(isExitKey(sf::Keyboard::Key::Escape) && !isExitKey(sf::Keyboard::Key::Q));
    for (int state = 0; state < 3; ++state) {                  // 0 running, 1 paused, 2 game over
        World w(12);
        startWorld(w);
        TickTimer t;
        PauseControl p;
        demoEat(w, 1);
        if (state == 1) p.toggle(w.session, t);
        if (state == 2) w.session.end(30, Collision::WALL);
        int calls = 0, savedScore = -1;
        ExitControl e;
        e.setOnExit([&] { ++calls; savedScore = w.session.isOver() ? w.session.finalScore() : w.score.value(); });
        assert(!e.requested());
        assert(e.request(state == 0 ? ExitReason::ESCAPE_KEY : ExitReason::WINDOW_CLOSED));
        assert(!e.request(ExitReason::ESCAPE_KEY));            // second request ignored
        assert(e.requested() && calls == 1);                   // hook ran exactly once
        assert(savedScore == (state == 2 ? 30 : 10));
    }
    ExitControl none;
    assert(!none.request(ExitReason::NONE) && !none.requested());
    std::cout << "US-12 OK: Esc / close button exit from running, paused and game over; hook runs once\n";
}

// ===================== US-13: Load High Score (FR-13, SEC-2, SEC-3, SO-3) =====================
// At start-up the stored high score is read from highscore.dat.
//  - Missing file  -> 0, no warning (normal on the first run).
//  - Valid file    -> the number: digits only, 0 .. 999 999, trailing spaces / newline (\n or \r\n) allowed.
//  - Anything else (letters, sign, decimal point, empty, > 6 digits, huge or binary file)
//                  -> warning on stderr and 0, the game still starts (SO-3 tamper resilience).
// The read is bounded to 16 characters (SEC-3), so a huge file cannot slow the start-up.
constexpr int    MAX_HIGH_SCORE = 999999;
constexpr size_t MAX_READ = 16;

enum class LoadStatus { OK, MISSING, INVALID, OUT_OF_RANGE, READ_ERROR };
struct HighScoreLoad {
    int value = 0;
    LoadStatus status = LoadStatus::MISSING;
};
inline const char* loadStatusName(LoadStatus s) {
    switch (s) { case LoadStatus::OK: return "ok"; case LoadStatus::MISSING: return "missing";
                 case LoadStatus::INVALID: return "invalid"; case LoadStatus::OUT_OF_RANGE: return "out of range";
                 default: return "read error"; }
}

// Warnings can be silenced while the self-tests feed in hundreds of broken files.
inline bool& highScoreWarnings() { static bool on = true; return on; }
inline void hsWarn(const std::string& msg) {
    if (highScoreWarnings()) std::cerr << "[warn] highscore: " << msg << '\n';
}

HighScoreLoad readHighScore(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) return {0, LoadStatus::MISSING};       // first run: normal, no warning

    std::string content;                                       // read at most MAX_READ+1 chars
    char ch;
    while (content.size() <= MAX_READ && in.get(ch)) content.push_back(ch);
    if (in.bad()) { hsWarn("read error, using 0"); return {0, LoadStatus::READ_ERROR}; }
    if (content.size() > MAX_READ) { hsWarn("file too long, resetting to 0"); return {0, LoadStatus::INVALID}; }

    while (!content.empty() && std::isspace(static_cast<unsigned char>(content.back())))
        content.pop_back();

    bool allDigits = !content.empty();
    for (char c : content)
        if (!std::isdigit(static_cast<unsigned char>(c))) { allDigits = false; break; }
    if (!allDigits) { hsWarn("invalid content, resetting to 0"); return {0, LoadStatus::INVALID}; }
    if (content.size() > 6) { hsWarn("out of range, resetting to 0"); return {0, LoadStatus::OUT_OF_RANGE}; }

    const int v = std::stoi(content);                          // safe: 1 .. 6 digits
    if (v > MAX_HIGH_SCORE) { hsWarn("out of range, resetting to 0"); return {0, LoadStatus::OUT_OF_RANGE}; }
    return {v, LoadStatus::OK};
}
int loadHighScore(const std::string& path) { return readHighScore(path).value; }

static void testUS13() {
    highScoreWarnings() = false;
    namespace fs = std::filesystem;
    const std::string f = (fs::temp_directory_path() / "snake_us13_test.dat").string();
    auto put = [&](const std::string& s) { std::ofstream o(f, std::ios::binary | std::ios::trunc); o << s; };
    auto is = [&](int v, LoadStatus st) { const auto r = readHighScore(f); return r.value == v && r.status == st; };
    std::error_code ec;
    fs::remove(f, ec);

    assert(is(0, LoadStatus::MISSING));                       // first run
    put("120\n");           assert(is(120, LoadStatus::OK));
    put("0");               assert(is(0, LoadStatus::OK));    // lower boundary
    put("999999");          assert(is(999999, LoadStatus::OK));   // upper boundary
    put("77\r\n");          assert(is(77, LoadStatus::OK));   // Windows line ending
    put("00042  \n");       assert(is(42, LoadStatus::OK));
    put("1000000");         assert(is(0, LoadStatus::OUT_OF_RANGE));
    put("");                assert(is(0, LoadStatus::INVALID));
    put("abc");             assert(is(0, LoadStatus::INVALID));
    put("-5");              assert(is(0, LoadStatus::INVALID));
    put("4.5");             assert(is(0, LoadStatus::INVALID));
    put("12 34");           assert(is(0, LoadStatus::INVALID));
    put(" 42");             assert(is(0, LoadStatus::INVALID));
    put(std::string(100000, '9'));        assert(is(0, LoadStatus::INVALID));   // bounded read
    put(std::string("\x00\xff\x10\x7f", 4)); assert(is(0, LoadStatus::INVALID));   // binary junk
    assert(loadHighScore(f) == 0);
    fs::remove(f, ec);
    highScoreWarnings() = true;
    std::cout << "US-13 OK: missing / valid / boundary / corrupt high-score files load safely\n";
}

// ===================== US-14: Update and Persist High Score (FR-14, SEC-1, SO-1, NFR-4) =====================
// When a session ends (game over, restart from pause, or exit) the final score is compared
// with the stored best and written only if it is strictly greater - the best never goes down.
// Writing is crash-safe: the number goes to highscore.dat.tmp first and is then renamed over
// the real file, so a crash or power cut mid-write leaves the old file intact (SO-1).
// If the platform cannot rename onto an existing file, the old file is moved aside first and
// put back if anything fails, so the old score is never lost.
bool saveHighScore(const std::string& path, int score) {
    namespace fs = std::filesystem;
    if (score < 0 || score > MAX_HIGH_SCORE) { hsWarn("refusing out-of-range value"); return false; }
    const std::string tmp = path + ".tmp";
    std::error_code ec;
    {
        std::ofstream out(tmp, std::ios::trunc | std::ios::binary);
        if (!out.is_open()) { hsWarn("cannot create temp file, old file kept"); return false; }
        out << score << '\n';
        out.flush();
        if (!out.good()) {
            out.close();
            fs::remove(tmp, ec);
            hsWarn("write failed, old file kept");
            return false;
        }
    }
    fs::rename(tmp, path, ec);
    if (ec) {                                                   // fallback: move old file aside
        const std::string bak = path + ".bak";
        std::error_code e2;
        fs::remove(bak, e2);
        fs::rename(path, bak, e2);
        const bool movedOld = !e2;
        fs::rename(tmp, path, ec);
        if (ec) {
            if (movedOld) fs::rename(bak, path, e2);            // put the old score back
            fs::remove(tmp, e2);
            hsWarn("rename failed, old file kept");
            return false;
        }
        if (movedOld) fs::remove(bak, e2);
    }
    return true;
}

// FR-14: called once at the end of a session. Returns the (possibly new) best.
int finalizeSession(const std::string& path, int storedHigh, int finalScore) {
    if (finalScore > storedHigh && saveHighScore(path, finalScore)) return finalScore;
    return storedHigh;
}

static void testUS14() {
    highScoreWarnings() = false;
    namespace fs = std::filesystem;
    const std::string f = (fs::temp_directory_path() / "snake_us14_test.dat").string();
    std::error_code ec;
    fs::remove(f, ec);

    assert(finalizeSession(f, 0, 50) == 50 && loadHighScore(f) == 50);       // first save
    assert(finalizeSession(f, 50, 30) == 50 && loadHighScore(f) == 50);      // lower: kept
    const auto stamp = fs::last_write_time(f);
    assert(finalizeSession(f, 50, 50) == 50 && fs::last_write_time(f) == stamp);   // equal: not rewritten
    assert(finalizeSession(f, 50, 999999) == 999999 && loadHighScore(f) == 999999); // upper boundary
    assert(!saveHighScore(f, 1000000) && !saveHighScore(f, -1));             // out of range refused
    assert(loadHighScore(f) == 999999);

    std::ifstream in(f, std::ios::binary);
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    assert(text == "999999\n");                                              // plain, portable text
    assert(!fs::exists(f + ".tmp") && !fs::exists(f + ".bak"));               // no leftovers

    const std::string bad = (fs::temp_directory_path() / "snake_no_such_dir_14" / "hs.dat").string();
    assert(finalizeSession(bad, 80, 200) == 80);                              // write fails: best kept

    fs::remove(f, ec);                                                        // NFR-4: many sessions
    std::mt19937 rng(14);
    int best = 0;
    for (int i = 0; i < 200; ++i) {
        const int s = static_cast<int>(rng() % 5000);
        const int nb = finalizeSession(f, best, s);
        assert(nb == std::max(best, s) && loadHighScore(f) == (nb > 0 ? nb : 0));
        best = nb;
    }
    fs::remove(f, ec);
    highScoreWarnings() = true;
    std::cout << "US-14 OK: best only goes up, atomic temp+rename write, failures keep the old value\n";
}

// Self-test: sprint-1 regression, then every sprint-2 story up to this one.
static void selfTest() {
    testSprint1();
    testUS10();
    testUS11();
    testUS12();
    testUS13();
    testUS14();
}

int main(int argc, char** argv) {
    selfTest();
    if (ui::hasFlag(argc, argv, "--test")) return 0;

    namespace fs = std::filesystem;
    const std::string shot = ui::argValue(argc, argv, "--shot");
    std::string file = "highscore.dat";
    if (!shot.empty()) {                                      // screenshot: a stored best of 20
        file = (fs::temp_directory_path() / "snake_us14_shot.dat").string();
        std::ofstream(file) << "20\n";
    }
    int best = loadHighScore(file);
    World w(shot.empty() ? std::random_device{}() : 14u);
    startWorld(w);
    TickTimer timer;
    PauseControl pause;
    ExitControl exitCtl;
    bool saved = false;                                       // FR-14: once per session
    std::string status = "Stored best: " + std::to_string(best);
    auto endSession = [&] {
        if (saved) return;
        saved = true;
        const int before = best;
        const int fin = w.session.isOver() ? w.session.finalScore() : w.score.value();
        best = finalizeSession(file, best, fin);
        status = best > before ? "New best " + std::to_string(best) + " saved"
                               : "Score " + std::to_string(fin) + ", best stays " + std::to_string(best);
        std::cout << "[save] " << status << '\n';
    };
    exitCtl.setOnExit(endSession);                            // quitting mid-game still saves
    auto restart = [&] {
        if (!canRestart(w.session, pause)) return;
        endSession();                                         // abandoned run from pause still counts
        restartGame(w, timer, pause);
        saved = false;
        status = "Stored best: " + std::to_string(best);
    };
    if (!shot.empty()) {                                      // screenshot: score 40, then crash
        demoEat(w, 4);
        while (!tickWorld(w)) {}
        endSession();
    }

    auto window = makeWindow("US-14 Update and Persist High Score");
    ui::Hud hud;
    if (!hud.load()) { std::cerr << "Error: could not load a font\n"; return 1; }
    sf::Clock frame;
    while (window.isOpen()) {
        while (const std::optional ev = window.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) { exitCtl.request(ExitReason::WINDOW_CLOSED); window.close(); }
            else if (ev->is<sf::Event::FocusLost>()) pause.pause(w.session, timer);
            else if (const auto* k = ev->getIf<sf::Event::KeyPressed>()) {
                if (isExitKey(k->code)) { exitCtl.request(ExitReason::ESCAPE_KEY); window.close(); }
                else if (isPauseKey(k->code)) pause.toggle(w.session, timer);
                else if (isRestartKey(k->code)) restart();
                else if (auto d = directionForKey(k->code)) steer(w, pause, *d);
            }
        }
        if (!window.isOpen()) break;
        if (updateWorld(w, timer, pause, frame.restart().asMilliseconds())) endSession();

        window.clear(ui::BG);
        drawScene(window, hud, w.snake, w.food, w.score.value(), best, status,
                  "Move: arrows/WASD   P: pause   R: restart   Esc: quit (saves)");
        if (w.session.isOver()) {
            hud.overlay(window, "GAME OVER", "Final score " + std::to_string(w.session.finalScore()) + "   -   R to restart");
            hud.centered(window, best > 0 && best == w.session.finalScore() ? "New high score saved to highscore.dat" : status, 16, best == w.session.finalScore() && best > 0 ? ui::GOLD : ui::MUTED, 312, true);
        } else if (pause.paused()) hud.overlay(window, "PAUSED", "P / Space to resume   -   R to restart");
        window.display();
        if (!shot.empty()) { ui::saveShot(window, shot); window.close(); }
    }
    if (!shot.empty()) { std::error_code ec; fs::remove(file, ec); }
}
