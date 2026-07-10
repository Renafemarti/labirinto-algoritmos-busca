#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <iostream>
#include <vector>

// Os headers de POO do seu projeto
#include "Maze.hpp"
#include "Renderer.hpp"
#include "Solver.hpp"

namespace {
    const char *algorithmName(Algorithm algo) {
        switch (algo) {
            case Algorithm::BFS:   return "BFS";
            case Algorithm::DFS:   return "DFS";
            case Algorithm::ASTAR: return "A*";
        }
        return "?";
    }

    // Callback para reajustar o viewport caso a janela seja redimensionada
    void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
        glViewport(0, 0, width, height);
    }
}

int main() {
    const int screenWidth = 900;
    const int screenHeight = 700;
    const int mazeWidth = 25;
    const int mazeHeight = 20;

    // 1. Inicialização do GLFW
    if (!glfwInit()) {
        std::cerr << "Falha ao inicializar o GLFW" << std::endl;
        return -1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 2. Criação da Janela
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Simulador de Labirinto - CG + POO (OpenGL)", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Falha ao criar a janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 3. Inicialização do GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Falha ao inicializar o GLAD" << std::endl;
        return -1;
    }

    // 4. Instanciação dos objetos (POO)
    Maze maze(mazeWidth, mazeHeight);
    maze.generate();

    Renderer renderer(screenWidth, screenHeight);
    renderer.computeCellSize(maze);
    
    // Inicializa os recursos visuais do OpenGL
    renderer.initOpenGLResources(); 

    Algorithm currentAlgo = Algorithm::BFS;
    SolveResult result = solve(maze, currentAlgo);

    // Variáveis de controle de animação
    size_t animatedStep = 0;
    float stepInterval = 0.02f; // Velocidade da animação
    bool animationDone = false;
    
    float lastFrameTime = glfwGetTime();
    float stepTimer = 0.0f;

    bool spacePressed = false, key1Pressed = false, key2Pressed = false, key3Pressed = false, keyRPressed = false;

    // Imprime as instruções iniciais no terminal do Linux/VSCode
    std::cout << "--- CONTROLES DO SIMULADOR ---\n";
    std::cout << "[1] BFS | [2] DFS | [3] A*\n";
    std::cout << "[ESPACO] Novo Labirinto | [R] Repetir Animacao\n";
    std::cout << "[Z] Mais Lento | [X] Mais Rapido\n";
    std::cout << "[SETAS] Mover Camera Livremente\n";
    std::cout << "Algoritmo atual: " << algorithmName(currentAlgo) << "\n\n";

    // 5. Loop de Jogo / Renderização
    while (!glfwWindowShouldClose(window)) {

        float currentFrameTime = glfwGetTime();
        float deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        // Input
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !spacePressed) {
            maze.generate();
            result = solve(maze, currentAlgo);
            animatedStep = 0; stepTimer = 0.0f; animationDone = false; spacePressed = true;
            std::cout << "Novo labirinto gerado. Algoritmo: " << algorithmName(currentAlgo) << "\n";
        } else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) spacePressed = false;

        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && !key1Pressed) {
            currentAlgo = Algorithm::BFS; result = solve(maze, currentAlgo);
            animatedStep = 0; stepTimer = 0.0f; animationDone = false; key1Pressed = true;
            std::cout << "Algoritmo alteredo para: BFS\n";
        } else if (glfwGetKey(window, GLFW_KEY_1) == GLFW_RELEASE) key1Pressed = false;

        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && !key2Pressed) {
            currentAlgo = Algorithm::DFS; result = solve(maze, currentAlgo);
            animatedStep = 0; stepTimer = 0.0f; animationDone = false; key2Pressed = true;
            std::cout << "Algoritmo alteredo para: DFS\n";
        } else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_RELEASE) key2Pressed = false;

        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS && !key3Pressed) {
            currentAlgo = Algorithm::ASTAR; result = solve(maze, currentAlgo);
            animatedStep = 0; stepTimer = 0.0f; animationDone = false; key3Pressed = true;
            std::cout << "Algoritmo alteredo para: A*\n";
        } else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_RELEASE) key3Pressed = false;

        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !keyRPressed) {
            animatedStep = 0; stepTimer = 0.0f; animationDone = false; keyRPressed = true;
            std::cout << "Animacao reiniciada.\n";
        } else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE) keyRPressed = false;

        // ==========================================
        // CONTROLE DE VELOCIDADE DA ANIMAÇÃO (Teclas Z e X)
        // ==========================================
        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
            stepInterval += 0.001f; // Aumenta o tempo de espera (Fica mais LENTO)
        }
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
            stepInterval -= 0.001f; // Diminui o tempo de espera (Fica mais RÁPIDO)
            if (stepInterval < 0.001f) stepInterval = 0.001f; // Limite de velocidade
        }

        // ==========================================
        // ATUALIZAÇÃO DA LÓGICA DA ANIMAÇÃO (Fase 1 + Fase 2)
        // ==========================================
        size_t totalAnimationSteps = result.visitOrder.size() + result.path.size();

        if (!animationDone) {
            stepTimer += deltaTime;
            while (stepTimer >= stepInterval && animatedStep < totalAnimationSteps) {
                animatedStep++;
                stepTimer -= stepInterval;
            }
            if (animatedStep >= totalAnimationSteps) {
                animationDone = true;
            }
        }

        // ==========================================
        // CONTROLES DE CÂMERA LIVRE
        // ==========================================
        float cameraSpeed = 90.0f * deltaTime; 
        
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  renderer.cameraYaw -= cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) renderer.cameraYaw += cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    renderer.cameraPitch += cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  renderer.cameraPitch -= cameraSpeed;

        if (renderer.cameraPitch > 89.0f) renderer.cameraPitch = 89.0f;
        if (renderer.cameraPitch < 1.0f)  renderer.cameraPitch = 1.0f;

        // ==========================================
        // DESENHO
        // ==========================================
        glClearColor(0.2f, 0.2f, 0.25f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderer.drawVisited(maze, result, animatedStep);
        renderer.drawPath(maze, result, animatedStep); 
        
        renderer.drawStartAndGoal(maze);
        renderer.drawMaze(maze);

        glfwSwapBuffers(window);
        glfwPollEvents(); // processar os inputs e fechar a janela
    }

    glfwTerminate();
    return 0;
}