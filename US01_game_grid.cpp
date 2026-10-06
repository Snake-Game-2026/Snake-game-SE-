// US-01: Initialize Game Grid  (FR-1, SEC-4)
// Build: g++ -std=c++17 US01_game_grid.cpp -o us01 && ./us01
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

struct Position { int col = 0, row = 0; };

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

int main() {
    GameGrid g;                                   // default 20x20
    assert(g.cols() == 20 && g.rows() == 20);
    assert(g.inBounds({0, 0}) && g.inBounds({19, 19}));
    assert(!g.inBounds({-1, 0}) && !g.inBounds({20, 5}) && !g.inBounds({5, 20}));
    assert(!g.setOccupied({25, 3}, true));        // rejected, no OOB write
    assert(g.setOccupied({3, 3}, true) && g.isOccupied({3, 3}));
    g.clear();
    assert(!g.isOccupied({3, 3}));

    GameGrid small(15, 15);                       // configurable
    assert(small.cols() == 15 && small.rows() == 15);
    GameGrid bad(2, 2);                           // invalid -> default
    assert(bad.cols() == 20);

    std::cout << "US-01 OK: grid " << g.cols() << "x" << g.rows() << "\n";
}
