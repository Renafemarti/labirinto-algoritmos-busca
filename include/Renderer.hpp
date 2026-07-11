#pragma once

#include "Maze.hpp"
#include "Solver.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <cstddef>
#include <string>
#include "stb_truetype.h"

class Renderer {
public:
    Renderer(int screenWidth, int screenHeight, int marginPx = 20);
    
    void computeCellSize(const Maze &maze);
    void initOpenGLResources(); 

    // Atualiza as dimensoes usadas para a projecao (aspect ratio) quando o
    // Canvas Swing/AWT que hospeda a janela GLFW embutida e redimensionado.
    // Nao mexe em nenhum buffer/textura, so nos valores usados para montar
    // a matriz de projecao em applyCameraAndLight()/drawTextOnFloor().
    void resize(int screenWidth, int screenHeight);

    void drawMaze(const Maze &maze) const;
    void drawVisited(const Maze &maze, const SolveResult &result, size_t stepCount) const;
    void drawPath(const Maze &maze, const SolveResult &result, size_t stepCount) const;
    void drawStartAndGoal(const Maze &maze) const;

    // Controla o ângulo da câmera (agora manipulados via mouse - ver motor.cpp)
    float cameraYaw;
    float cameraPitch;
    float cameraZoom = 1.0f; // 1.0 = distancia padrao; <1 aproxima, >1 afasta (scroll do mouse)

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

    unsigned int textShaderProgram;
    unsigned int textVAO, textVBO;
    unsigned int fontTexture;
    stbtt_bakedchar cdata[96];

    void initText();
    void drawTextOnFloor(std::string text, float cx, float cz, float scale, glm::vec3 color) const;
};