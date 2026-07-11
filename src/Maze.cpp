#include "Maze.hpp"

#include <algorithm>
#include <random>
#include <stack>

Maze::Maze(int width, int height)
    : width_(width), height_(height), cells_(static_cast<size_t>(width) * height) {}

bool Maze::inBounds(int x, int y) const {
    return x >= 0 && x < width_ && y >= 0 && y < height_;
}

bool Maze::canMove(int x, int y, Direction dir) const {
    int nx = x, ny = y;
    switch (dir) {
        case NORTH: ny -= 1; break;
        case EAST:  nx += 1; break;
        case SOUTH: ny += 1; break;
        case WEST:  nx -= 1; break;
    }
    if (!inBounds(nx, ny)) return false;
    return !at(x, y).walls[dir];
}

void Maze::removeWallBetween(int x1, int y1, int x2, int y2) {
    if (x2 == x1 + 1) { at(x1, y1).walls[EAST] = false;  at(x2, y2).walls[WEST]  = false; }
    if (x2 == x1 - 1) { at(x1, y1).walls[WEST] = false;  at(x2, y2).walls[EAST]  = false; }
    if (y2 == y1 + 1) { at(x1, y1).walls[SOUTH] = false; at(x2, y2).walls[NORTH] = false; }
    if (y2 == y1 - 1) { at(x1, y1).walls[NORTH] = false; at(x2, y2).walls[SOUTH] = false; }
}

void Maze::generate(unsigned int seed) {
    // Reseta o estado (útil se quiserem gerar várias vezes em runtime)
    for (auto &c : cells_) {
        c = Cell{};
    }

    std::mt19937 rng(seed == 0 ? std::random_device{}() : seed);

    struct StackEntry { int x, y; };
    std::stack<StackEntry> stack;

    at(0, 0).visited = true;
    stack.push({0, 0});

    // Deslocamentos correspondentes a NORTH, EAST, SOUTH, WEST
    const int dx[4] = {0, 1, 0, -1};
    const int dy[4] = {-1, 0, 1, 0};

    while (!stack.empty()) {
        auto [x, y] = stack.top();

        // Lista de vizinhos ainda não visitados
        std::vector<int> candidates;
        for (int dir = 0; dir < 4; ++dir) {
            int nx = x + dx[dir];
            int ny = y + dy[dir];
            if (inBounds(nx, ny) && !at(nx, ny).visited) {
                candidates.push_back(dir);
            }
        }

        if (candidates.empty()) {
            stack.pop(); // beco sem saída -> backtrack
            continue;
        }

        std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
        int dir = candidates[dist(rng)];
        int nx = x + dx[dir];
        int ny = y + dy[dir];

        removeWallBetween(x, y, nx, ny);
        at(nx, ny).visited = true;
        stack.push({nx, ny});
    }
}
