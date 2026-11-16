// US-01 - Initialize Game Grid (FR-1)  [GitHub #35, In Progress]
// SFML 3.x, C++17
// Build: g++ -std=c++17 US01_initialize_game_grid.cpp -o US01_initialize_game_grid -lsfml-graphics -lsfml-window -lsfml-system
// Run:   ./US01_initialize_game_grid            (opens the demo window; self-test runs first)
//        ./US01_initialize_game_grid --test     (self-test only, no window)

#include <SFML/Graphics.hpp>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <string>
#include <system_error>
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
    bool load() {
        const char* p[] = {"C:/Windows/Fonts/segoeui.ttf", "C:/Windows/Fonts/arial.ttf",
                           "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                           "/usr/share/fonts/TTF/DejaVuSans.ttf",
                           "/System/Library/Fonts/Supplemental/Arial.ttf"};
        for (auto f : p) if (font.openFromFile(f)) return true;
        return false;
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

static void selfTest() {
    GameGrid g;
    assert(g.cols() == 20 && g.rows() == 20);                 // FR-1 default size
    assert(g.inBounds({0, 0}) && g.inBounds({19, 19}));
    assert(!g.inBounds({-1, 0}) && !g.inBounds({20, 5}) && !g.inBounds({5, 20}));
    assert(!g.setOccupied({25, 3}, true));                    // SEC-4 out-of-range write refused
    assert(!g.isOccupied({-3, -3}));                          // SEC-4 out-of-range read safe
    assert(g.setOccupied({3, 3}, true) && g.isOccupied({3, 3}));
    g.clear();
    assert(!g.isOccupied({3, 3}));
    GameGrid small(15, 15);                                   // configurable
    assert(small.cols() == 15 && small.rows() == 15);
    GameGrid bad(2, 2);                                       // too small -> default
    assert(bad.cols() == 20 && bad.rows() == 20);
    std::cout << "US-01 OK: grid " << g.cols() << "x" << g.rows() << "\n";
}

int main(int argc, char** argv) {
    selfTest();
    if (ui::hasFlag(argc, argv, "--test")) return 0;

    GameGrid grid;                                   // default 20x20
    auto window = makeWindow("US-01 Initialize Game Grid");
    ui::Hud hud;
    if (!hud.load()) { std::cerr << "Error: could not load a font\n"; return 1; }
    const std::string shot = ui::argValue(argc, argv, "--shot");

    while (window.isOpen()) {
        while (const std::optional ev = window.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) window.close();
            else if (const auto* k = ev->getIf<sf::Event::KeyPressed>())
                if (k->code == sf::Keyboard::Key::Escape) window.close();
        }
        window.clear(ui::BG);
        hud.draw(window, 0, 0, "US-01  grid " + std::to_string(grid.cols()) + " x " + std::to_string(grid.rows()));
        hud.hint(window, "Esc: quit");
        ui::drawBoard(window);
        window.display();
        if (!shot.empty()) { ui::saveShot(window, shot); window.close(); }
    }
}
