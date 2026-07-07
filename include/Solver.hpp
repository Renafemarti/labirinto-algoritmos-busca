#pragma once

#include <vector>
#include "Maze.hpp"

enum class Algorithm { BFS, DFS, ASTAR };

// Resultado completo de uma busca, guardado passo a passo para permitir
// animação: 'visitOrder' é a ordem em que os nós foram explorados (isso
// é o que vocês vão desenhar célula por célula, com um pequeno delay),
// e 'path' é o caminho final da entrada até a saída (vazio se não achou).
struct SolveResult {
    std::vector<int> visitOrder; // índices de célula (Maze::index) na ordem de exploração
    std::vector<int> path;       // índices de célula do start até o goal, em ordem
};

// Executa o algoritmo escolhido do início ao fim e devolve o resultado
// completo já pronto para ser "reproduzido" na tela pelo Renderer.
// Ideia de uso na visualização: percorram 'visitOrder' pintando cada
// célula de uma cor, e ao final desenhem 'path' por cima em outra cor.
SolveResult solve(const Maze &maze, Algorithm algo);
