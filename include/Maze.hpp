#pragma once

#include <cstdint>
#include <vector>

// Direções cardeais usadas como índice de parede/vizinho.
// A ordem é fixa: Norte, Leste, Sul, Oeste.
enum Direction : int { NORTH = 0, EAST = 1, SOUTH = 2, WEST = 3 };

struct Cell {
    // walls[NORTH], walls[EAST], walls[SOUTH], walls[WEST]
    // true  = existe parede (não pode passar)
    // false = parede foi removida (passagem aberta)
    bool walls[4] = {true, true, true, true};
    bool visited = false; // usado durante a geração do labirinto
};

// Labirinto representado como uma grade width x height de células.
// Índice linear: index(x, y) = y * width + x
class Maze {
public:
    Maze(int width, int height);

    // Gera um labirinto perfeito (sem loops, sempre solúvel) usando
    // o algoritmo de "recursive backtracking" (busca em profundidade
    // com pilha explícita, evitando estouro de pilha em labirintos grandes).
    void generate(unsigned int seed = 0);

    int width() const { return width_; }
    int height() const { return height_; }

    const Cell &at(int x, int y) const { return cells_[index(x, y)]; }
    Cell &at(int x, int y) { return cells_[index(x, y)]; }

    bool inBounds(int x, int y) const;
    int index(int x, int y) const { return y * width_ + x; }

    // Retorna true se é possível andar da célula (x,y) na direção 'dir'
    // (ou seja, não há parede naquele lado).
    bool canMove(int x, int y, Direction dir) const;

    // Posições padrão de entrada/saída (canto superior-esquerdo e
    // inferior-direito). Podem ser trocadas depois se quiserem algo
    // mais elaborado (ex.: pontos aleatórios nas bordas).
    std::pair<int, int> start() const { return {0, 0}; }
    std::pair<int, int> goal() const { return {width_ - 1, height_ - 1}; }

private:
    int width_;
    int height_;
    std::vector<Cell> cells_;

    void removeWallBetween(int x1, int y1, int x2, int y2);
};
