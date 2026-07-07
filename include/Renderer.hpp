#pragma once

#include "Maze.hpp"
#include "Solver.hpp"
#include <glm/glm.hpp>
#include <cstddef>

class Renderer {
public:
    Renderer(int screenWidth, int screenHeight, int marginPx = 20);
    
    void computeCellSize(const Maze &maze);
    void drawMaze(const Maze &maze) const;
    void drawVisited(const Maze &maze, const SolveResult &result, size_t stepCount) const;
    void drawPath(const Maze &maze, const SolveResult &result) const;
    void drawStartAndGoal(const Maze &maze) const;

    // Nova função crucial para inicializar o OpenGL
    void initOpenGLResources(); 

private:
    int screenWidth_;
    int screenHeight_;
    int margin_;
    float cellSize_;

    // Variáveis de controle do OpenGL
    unsigned int shaderProgram;
    unsigned int quadVAO, quadVBO;
    unsigned int lineVAO, lineVBO;

    void cellTopLeft(int x, int y, float &outX, float &outY) const;
    
    // Funções internas que substituem as do Raylib
    void drawQuad(float x, float y, float w, float h, glm::vec4 color) const;
    void drawLine(float x1, float y1, float x2, float y2, float thickness, glm::vec4 color) const;
};