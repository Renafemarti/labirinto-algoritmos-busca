#pragma once

#include "Maze.hpp"
#include "Solver.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <cstddef>

class Renderer {
public:
    Renderer(int screenWidth, int screenHeight, int marginPx = 20);
    
    void computeCellSize(const Maze &maze);
    void initOpenGLResources(); 

    void drawMaze(const Maze &maze) const;
    void drawVisited(const Maze &maze, const SolveResult &result, size_t stepCount) const;
    void drawPath(const Maze &maze, const SolveResult &result, size_t stepCount) const;
    void drawStartAndGoal(const Maze &maze) const;

    // Controla o ângulo da câmera
    float cameraYaw;
    float cameraPitch;

private:
    int screenWidth_;
    int screenHeight_;
    int margin_;
    float cellSize_;
    float mazeWidth3D_, mazeDepth3D_;

    unsigned int shaderProgram;
    
    // Buffers para o Cubo (Paredes)
    unsigned int cubeVAO, cubeVBO;
    
    // Buffers para a Esfera (Bola/Caminhos)
    unsigned int sphereVAO, sphereVBO, sphereEBO;
    int sphereIndexCount;

    // Funções internas de geração e desenho
    void setupCube();
    void setupSphere(int sectorCount, int stackCount);
    void getCellCenter(int x, int y, float &outX, float &outZ) const;
    
    void drawCube(glm::vec3 position, glm::vec3 scale, glm::vec4 color) const;
    void drawSphere(glm::vec3 position, float radius, glm::vec4 color) const;
    void applyCameraAndLight() const;
};