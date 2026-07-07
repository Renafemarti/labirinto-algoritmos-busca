#include "Renderer.hpp"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <algorithm>

// Shaders básicos para desenhar formas 2D com cores sólidas
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    uniform mat4 projection;
    uniform mat4 model;
    void main() {
        gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    uniform vec4 color;
    void main() {
        FragColor = color;
    }
)";

Renderer::Renderer(int screenWidth, int screenHeight, int marginPx)
    : screenWidth_(screenWidth), screenHeight_(screenHeight), margin_(marginPx),
      shaderProgram(0), quadVAO(0), quadVBO(0), lineVAO(0), lineVBO(0) {}

void Renderer::initOpenGLResources() {
    // Compilação dos Shaders
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Configuração do Retângulo (Quad) para desenhar células
    float quadVertices[] = {
        0.0f, 0.0f, // Top-left
        1.0f, 0.0f, // Top-right
        0.0f, 1.0f, // Bottom-left
        1.0f, 0.0f, // Top-right
        1.0f, 1.0f, // Bottom-right
        0.0f, 1.0f  // Bottom-left
    };
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Configuração da Linha Dinâmica para desenhar paredes
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(float), NULL, GL_DYNAMIC_DRAW); 
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Habilita transparência (para o algoritmo de busca ficar semi-transparente)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::drawQuad(float x, float y, float w, float h, glm::vec4 color) const {
    glUseProgram(shaderProgram);

    // Matriz Ortográfica (faz a coordenada 0,0 ser no topo esquerdo da tela)
    glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth_, (float)screenHeight_, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(x, y, 0.0f));
    model = glm::scale(model, glm::vec3(w, h, 1.0f));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    
    glUniform4fv(glGetUniformLocation(shaderProgram, "color"), 1, glm::value_ptr(color));

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Renderer::drawLine(float x1, float y1, float x2, float y2, float thickness, glm::vec4 color) const {
    glUseProgram(shaderProgram);

    glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth_, (float)screenHeight_, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform4fv(glGetUniformLocation(shaderProgram, "color"), 1, glm::value_ptr(color));

    float vertices[] = { x1, y1, x2, y2 };
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

    glLineWidth(thickness);
    glDrawArrays(GL_LINES, 0, 2);
}

void Renderer::computeCellSize(const Maze &maze) {
    float availableW = static_cast<float>(screenWidth_ - 2 * margin_);
    float availableH = static_cast<float>(screenHeight_ - 2 * margin_);
    float cellW = availableW / static_cast<float>(maze.width());
    float cellH = availableH / static_cast<float>(maze.height());
    cellSize_ = std::min(cellW, cellH);
}

void Renderer::cellTopLeft(int x, int y, float &outX, float &outY) const {
    outX = margin_ + x * cellSize_;
    outY = margin_ + y * cellSize_;
}

void Renderer::drawMaze(const Maze &maze) const {
    glm::vec4 wallColor(0.0f, 0.0f, 0.0f, 1.0f); // BLACK
    const float thickness = 2.0f;

    for (int y = 0; y < maze.height(); ++y) {
        for (int x = 0; x < maze.width(); ++x) {
            float px, py;
            cellTopLeft(x, y, px, py);
            const Cell &cell = maze.at(x, y);

            if (cell.walls[NORTH]) drawLine(px, py, px + cellSize_, py, thickness, wallColor);
            if (cell.walls[WEST])  drawLine(px, py, px, py + cellSize_, thickness, wallColor);
            if (cell.walls[EAST])  drawLine(px + cellSize_, py, px + cellSize_, py + cellSize_, thickness, wallColor);
            if (cell.walls[SOUTH]) drawLine(px, py + cellSize_, px + cellSize_, py + cellSize_, thickness, wallColor);
        }
    }
}

void Renderer::drawVisited(const Maze &maze, const SolveResult &result, size_t stepCount) const {
    glm::vec4 visitedColor(135.0f/255.0f, 206.0f/255.0f, 250.0f/255.0f, 180.0f/255.0f); // Azul semi-transparente
    float pad = cellSize_ * 0.15f;

    size_t limit = std::min(stepCount, result.visitOrder.size());
    for (size_t i = 0; i < limit; ++i) {
        int idx = result.visitOrder[i];
        int x = idx % maze.width();
        int y = idx / maze.width();
        float px, py;
        cellTopLeft(x, y, px, py);
        drawQuad(px + pad, py + pad, cellSize_ - 2 * pad, cellSize_ - 2 * pad, visitedColor);
    }
}

void Renderer::drawPath(const Maze &maze, const SolveResult &result) const {
    glm::vec4 pathColor(50.0f/255.0f, 180.0f/255.0f, 50.0f/255.0f, 1.0f); // Verde
    float pad = cellSize_ * 0.30f;

    for (int idx : result.path) {
        int x = idx % maze.width();
        int y = idx / maze.width();
        float px, py;
        cellTopLeft(x, y, px, py);
        drawQuad(px + pad, py + pad, cellSize_ - 2 * pad, cellSize_ - 2 * pad, pathColor);
    }
}

void Renderer::drawStartAndGoal(const Maze &maze) const {
    auto [sx, sy] = maze.start();
    auto [gx, gy] = maze.goal();

    glm::vec4 startColor(1.0f, 215.0f/255.0f, 0.0f, 1.0f); // Dourado
    glm::vec4 goalColor(220.0f/255.0f, 20.0f/255.0f, 60.0f/255.0f, 1.0f); // Vermelho

    float px, py;
    cellTopLeft(sx, sy, px, py);
    drawQuad(px, py, cellSize_, cellSize_, startColor);

    cellTopLeft(gx, gy, px, py);
    drawQuad(px, py, cellSize_, cellSize_, goalColor);
}