#include "Solver.hpp"

#include <algorithm>
#include <cmath>
#include <deque>
#include <limits>
#include <queue>
#include <unordered_map>

namespace {

std::vector<int> reconstructPath(const std::unordered_map<int, int> &cameFrom,
                                  int startIdx, int goalIdx) {
    std::vector<int> path;
    if (startIdx == goalIdx) {
        path.push_back(startIdx);
        return path;
    }
    if (cameFrom.find(goalIdx) == cameFrom.end()) {
        return path; // sem caminho encontrado
    }
    int current = goalIdx;
    while (current != startIdx) {
        path.push_back(current);
        current = cameFrom.at(current);
    }
    path.push_back(startIdx);
    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<int> neighbors(const Maze &maze, int x, int y) {
    std::vector<int> result;
    const Direction dirs[4] = {NORTH, EAST, SOUTH, WEST};
    const int dx[4] = {0, 1, 0, -1};
    const int dy[4] = {-1, 0, 1, 0};
    for (int i = 0; i < 4; ++i) {
        if (maze.canMove(x, y, dirs[i])) {
            result.push_back(maze.index(x + dx[i], y + dy[i]));
        }
    }
    return result;
}

SolveResult bfs(const Maze &maze, int startIdx, int goalIdx) {
    SolveResult result;
    std::vector<bool> visited(static_cast<size_t>(maze.width()) * maze.height(), false);
    std::unordered_map<int, int> cameFrom;
    std::queue<int> frontier;

    frontier.push(startIdx);
    visited[startIdx] = true;
    result.maxNodesInMemory = 1;

    while (!frontier.empty()) {
        result.iterations++;

        int current = frontier.front();
        frontier.pop();
        result.visitOrder.push_back(current);
        result.nodesExpanded++;

        if (current == goalIdx) break;

        int x = current % maze.width();
        int y = current / maze.width();
        for (int next : neighbors(maze, x, y)) {
            if (!visited[next]) {
                visited[next] = true;
                cameFrom[next] = current;
                frontier.push(next);
            }
        }

        result.maxNodesInMemory = std::max(result.maxNodesInMemory,
                                            static_cast<int>(frontier.size()));
    }

    result.path = reconstructPath(cameFrom, startIdx, goalIdx);
    result.cost = result.path.empty() ? -1 : static_cast<int>(result.path.size()) - 1;
    return result;
}

SolveResult dfs(const Maze &maze, int startIdx, int goalIdx) {
    SolveResult result;
    std::vector<bool> visited(static_cast<size_t>(maze.width()) * maze.height(), false);
    std::unordered_map<int, int> cameFrom;
    std::vector<int> stack;

    stack.push_back(startIdx);
    result.maxNodesInMemory = 1;

    while (!stack.empty()) {
        result.iterations++;

        int current = stack.back();
        stack.pop_back();

        if (visited[current]) continue;
        visited[current] = true;
        result.visitOrder.push_back(current);
        result.nodesExpanded++;

        if (current == goalIdx) break;

        int x = current % maze.width();
        int y = current / maze.width();
        for (int next : neighbors(maze, x, y)) {
            if (!visited[next]) {
                if (cameFrom.find(next) == cameFrom.end()) {
                    cameFrom[next] = current;
                }
                stack.push_back(next);
            }
        }

        result.maxNodesInMemory = std::max(result.maxNodesInMemory,
                                            static_cast<int>(stack.size()));
    }

    result.path = reconstructPath(cameFrom, startIdx, goalIdx);
    result.cost = result.path.empty() ? -1 : static_cast<int>(result.path.size()) - 1;
    return result;
}

SolveResult astar(const Maze &maze, int startIdx, int goalIdx) {
    SolveResult result;

    auto heuristic = [&](int idx) {
        int x = idx % maze.width();
        int y = idx / maze.width();
        int gx = goalIdx % maze.width();
        int gy = goalIdx / maze.width();
        // Distância de Manhattan: admissível para movimento em grade 4-direções
        return std::abs(x - gx) + std::abs(y - gy);
    };

    using QueueEntry = std::pair<int, int>; // (fScore, index)
    std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<>> frontier;

    std::vector<int> gScore(static_cast<size_t>(maze.width()) * maze.height(),
                             std::numeric_limits<int>::max());
    std::vector<bool> closed(gScore.size(), false);
    std::unordered_map<int, int> cameFrom;

    gScore[startIdx] = 0;
    frontier.push({heuristic(startIdx), startIdx});
    result.maxNodesInMemory = 1;

    while (!frontier.empty()) {
        result.iterations++;

        auto [f, current] = frontier.top();
        (void)f;
        frontier.pop();

        if (closed[current]) continue;
        closed[current] = true;
        result.visitOrder.push_back(current);
        result.nodesExpanded++;

        if (current == goalIdx) break;

        int x = current % maze.width();
        int y = current / maze.width();
        for (int next : neighbors(maze, x, y)) {
            int tentativeG = gScore[current] + 1; // custo uniforme por passo
            if (tentativeG < gScore[next]) {
                gScore[next] = tentativeG;
                cameFrom[next] = current;
                frontier.push({tentativeG + heuristic(next), next});
            }
        }

        result.maxNodesInMemory = std::max(result.maxNodesInMemory,
                                            static_cast<int>(frontier.size()));
    }

    result.path = reconstructPath(cameFrom, startIdx, goalIdx);
    result.cost = result.path.empty() ? -1 : gScore[goalIdx];
    return result;
}

} // namespace

SolveResult solve(const Maze &maze, Algorithm algo) {
    auto [sx, sy] = maze.start();
    auto [gx, gy] = maze.goal();
    int startIdx = maze.index(sx, sy);
    int goalIdx = maze.index(gx, gy);

    switch (algo) {
        case Algorithm::BFS:   return bfs(maze, startIdx, goalIdx);
        case Algorithm::DFS:   return dfs(maze, startIdx, goalIdx);
        case Algorithm::ASTAR: return astar(maze, startIdx, goalIdx);
    }
    return {};
}