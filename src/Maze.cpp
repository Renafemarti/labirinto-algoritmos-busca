#include "Maze.hpp"

#include <algorithm>
#include <functional>
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

void Maze::generate(unsigned int seed, MazeAlgorithm algo) {
    // Reseta o estado (útil se quiserem gerar várias vezes em runtime)
    for (auto &c : cells_) {
        c = Cell{};
    }

    switch (algo) {
        case MazeAlgorithm::RECURSIVE_BACKTRACKING:
            generateRecursiveBacktracking(seed);
            break;
        case MazeAlgorithm::PRIM:
            generatePrim(seed);
            break;
        case MazeAlgorithm::KRUSKAL:
            generateKruskal(seed);
            break;
    }
}

// -----------------------------------------------------------------
// Recursive Backtracking (DFS aleatorizado com pilha explicita).
// -----------------------------------------------------------------
void Maze::generateRecursiveBacktracking(unsigned int seed) {
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

// -----------------------------------------------------------------
// Prim aleatorizado.
//
// Mantemos uma lista de "arestas de fronteira": pares (celula-visitada,
// celula-vizinha-nao-visitada). A cada passo sorteamos UMA aresta
// qualquer dessa lista (nao necessariamente a mais recente, diferente
// de uma pilha/fila), removemos a parede correspondente, marcamos a
// celula nova como visitada e empurramos as arestas dela para a
// fronteira. Isso da um crescimento mais "arbustivo" (bushy), com mais
// bifurcacoes curtas, ao contrario dos corredores longos do
// backtracking.
// -----------------------------------------------------------------
void Maze::generatePrim(unsigned int seed) {
    std::mt19937 rng(seed == 0 ? std::random_device{}() : seed);

    const int dx[4] = {0, 1, 0, -1};
    const int dy[4] = {-1, 0, 1, 0};

    struct FrontierEdge { int fromX, fromY, toX, toY; };
    std::vector<FrontierEdge> frontier;

    auto addFrontierCellsOf = [&](int x, int y) {
        for (int dir = 0; dir < 4; ++dir) {
            int nx = x + dx[dir];
            int ny = y + dy[dir];
            if (inBounds(nx, ny) && !at(nx, ny).visited) {
                frontier.push_back({x, y, nx, ny});
            }
        }
    };

    at(0, 0).visited = true;
    addFrontierCellsOf(0, 0);

    while (!frontier.empty()) {
        std::uniform_int_distribution<size_t> dist(0, frontier.size() - 1);
        size_t idx = dist(rng);
        FrontierEdge edge = frontier[idx];

        // Remove a aresta escolhida da lista trocando com a ultima (O(1),
        // ordem nao importa aqui).
        frontier[idx] = frontier.back();
        frontier.pop_back();

        // A celula de destino pode ter sido visitada nesse meio tempo por
        // outra aresta da fronteira que apontava para ela tambem; nesse
        // caso so descartamos esta aresta (evita reconectar/criar loop).
        if (at(edge.toX, edge.toY).visited) continue;

        removeWallBetween(edge.fromX, edge.fromY, edge.toX, edge.toY);
        at(edge.toX, edge.toY).visited = true;
        addFrontierCellsOf(edge.toX, edge.toY);
    }
}

// -----------------------------------------------------------------
// Kruskal aleatorizado, com Union-Find (disjoint set) para detectar
// ciclos.
//
// Listamos TODAS as paredes internas da grade (entre celulas vizinhas)
// como possiveis "arestas", embaralhamos essa lista inteira e vamos
// removendo parede por parede: uma parede so pode ser removida se as
// duas celulas que ela separa ainda NAO estiverem no mesmo conjunto
// (senao formaria um loop). Ao contrario do Prim/backtracking, que
// crescem a partir de uma unica celula "semente", o Kruskal trabalha
// com a grade inteira desde o inicio, o que costuma dar um padrao mais
// uniforme e sem vies de direcao.
// -----------------------------------------------------------------
void Maze::generateKruskal(unsigned int seed) {
    std::mt19937 rng(seed == 0 ? std::random_device{}() : seed);

    // Union-Find simples com path compression + union by rank.
    std::vector<int> parent(cells_.size());
    std::vector<int> rank_(cells_.size(), 0);
    for (size_t i = 0; i < parent.size(); ++i) parent[i] = static_cast<int>(i);

    std::function<int(int)> find = [&](int a) {
        while (parent[a] != a) {
            parent[a] = parent[parent[a]]; // path compression
            a = parent[a];
        }
        return a;
    };

    auto unite = [&](int a, int b) {
        a = find(a);
        b = find(b);
        if (a == b) return;
        if (rank_[a] < rank_[b]) std::swap(a, b);
        parent[b] = a;
        if (rank_[a] == rank_[b]) rank_[a]++;
    };

    // Cada aresta = parede entre duas celulas adjacentes. So precisamos
    // listar EAST e SOUTH de cada celula para cobrir toda a grade sem
    // duplicar (a parede WEST de (x+1,y) e a mesma que a EAST de (x,y)).
    struct Edge { int x1, y1, x2, y2; };
    std::vector<Edge> edges;
    edges.reserve(static_cast<size_t>(width_) * height_ * 2);

    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            if (x + 1 < width_) edges.push_back({x, y, x + 1, y});
            if (y + 1 < height_) edges.push_back({x, y, x, y + 1});
        }
    }

    std::shuffle(edges.begin(), edges.end(), rng);

    for (const Edge &e : edges) {
        int idxA = index(e.x1, e.y1);
        int idxB = index(e.x2, e.y2);
        if (find(idxA) != find(idxB)) {
            removeWallBetween(e.x1, e.y1, e.x2, e.y2);
            unite(idxA, idxB);
        }
    }

    // 'visited' nao e usado pelo Kruskal (nao ha nocao de "frente de
    // avanco"), mas marcamos tudo como true por consistencia com os
    // outros dois algoritmos, caso algum outro codigo dependa disso.
    for (auto &c : cells_) c.visited = true;
}