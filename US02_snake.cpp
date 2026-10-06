// US-02: Initialize Snake  (FR-2)
// Snake at grid centre, length 3, heading RIGHT. Body stored head-first.
// Build: g++ -std=c++17 US02_snake.cpp -o us02 && ./us02
#include <cassert>
#include <deque>
#include <iostream>

struct Position {
    int col = 0, row = 0;
    bool operator==(const Position& o) const { return col == o.col && row == o.row; }
};
enum class Direction { UP, DOWN, LEFT, RIGHT };

class Snake {
public:
    static constexpr size_t INITIAL_LENGTH = 3;

    void init(int gridCols, int gridRows) {
        body_.clear();
        heading_ = Direction::RIGHT;
        const int c = gridCols / 2, r = gridRows / 2;
        for (size_t i = 0; i < INITIAL_LENGTH; ++i)
            body_.push_back({c - static_cast<int>(i), r});   // head first, tail to the left
    }
    const std::deque<Position>& body() const { return body_; }
    Position head() const { return body_.front(); }
    Direction heading() const { return heading_; }
    size_t length() const { return body_.size(); }

private:
    std::deque<Position> body_;
    Direction heading_ = Direction::RIGHT;
};

int main() {
    Snake s;
    s.init(20, 20);
    assert(s.length() == 3);
    assert(s.heading() == Direction::RIGHT);
    assert(s.head() == (Position{10, 10}));
    assert(s.body()[1] == (Position{9, 10}));
    assert(s.body()[2] == (Position{8, 10}));

    s.init(15, 15);                                // re-init (used on restart)
    assert(s.head() == (Position{7, 7}) && s.length() == 3);

    std::cout << "US-02 OK: head at (" << s.head().col << "," << s.head().row << ")\n";
}
