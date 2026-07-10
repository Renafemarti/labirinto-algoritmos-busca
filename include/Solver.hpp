#pragma once
#include <vector>
#include "Maze.hpp"

// BFS      - Busca em Largura
// DFS      - Busca em Profundidade
// ASTAR    - A* (g + heurística Manhattan)
// DIJKSTRA - Dijkstra (custo uniforme, sem heurística; equivalente a UCS)
// GREEDY   - Busca Gulosa / Greedy Best-First Search (apenas heurística)
enum class Algorithm { BFS, DFS, ASTAR, DIJKSTRA, GREEDY };

// Resultado completo de uma busca, guardado passo a passo para permitir
// animação: 'visitOrder' é a ordem em que os nós foram explorados (isso
// é o que vocês vão desenhar célula por célula, com um pequeno delay),
// e 'path' é o caminho final da entrada até a saída (vazio se não achou).
//
// Também guardamos métricas de desempenho para comparar os algoritmos
// depois em um gráfico:
//   - cost:             custo total do caminho encontrado (-1 se não houve caminho)
//   - nodesExpanded:    quantidade de nós efetivamente processados (expandidos)
//   - maxNodesInMemory: maior quantidade de nós na fronteira/pilha/fila
//                        durante toda a busca (uso de memória)
//   - iterations:       número de iterações do laço principal da busca
struct SolveResult {
    std::vector<int> visitOrder; // índices de célula (Maze::index) na ordem de exploração
    std::vector<int> path;       // índices de célula do start até o goal, em ordem

    int cost = -1;
    int nodesExpanded = 0;
    int maxNodesInMemory = 0;
    int iterations = 0;
};

// Executa o algoritmo escolhido do início ao fim e devolve o resultado
// completo já pronto para ser "reproduzido" na tela pelo Renderer.
// Ideia de uso na visualização: percorram 'visitOrder' pintando cada
// célula de uma cor, e ao final desenhem 'path' por cima em outra cor.
SolveResult solve(const Maze &maze, Algorithm algo);