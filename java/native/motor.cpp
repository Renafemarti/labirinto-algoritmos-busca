#include <jni.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <atomic>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>

#include "Maze.hpp"
#include "Renderer.hpp"
#include "Solver.hpp"

#include "../headers/main_MotorGrafico.h"

// ---------------------------------------------------------------------
// Estado global do motor. Escrito pela thread Swing (via JNI) e lido
// pela thread OpenGL a cada frame. Por isso os valores compartilhados
// usam std::atomic (leitura/escrita simples e sem necessidade de mutex).
// A unica excecao e o caminho do arquivo a salvar, que e uma std::string
// (nao existe std::atomic<std::string>), protegida por um mutex.
// ---------------------------------------------------------------------

namespace {

GLFWwindow *window = nullptr;
Maze *maze = nullptr;
Renderer *renderer = nullptr;

SolveResult result;
Algorithm currentAlgo = Algorithm::BFS;

size_t animatedStep = 0;
float stepTimer = 0.0f;
bool animationDone = false;

std::atomic<float> g_stepInterval{0.02f};
std::atomic<float> g_cameraYaw{45.0f};
std::atomic<float> g_cameraPitch{45.0f};

std::atomic<bool> g_requestNewMaze{false};
std::atomic<bool> g_requestReset{false};
std::atomic<int> g_requestAlgorithm{-1}; // -1 = nenhuma solicitacao pendente
std::atomic<bool> g_shouldClose{false};

std::atomic<bool> g_requestSave{false};
std::mutex g_savePathMutex;
std::string g_savePath;

// pause e volta
std::atomic<bool> g_isPaused{false};
std::atomic<bool> g_requestStepBack{false};

Algorithm algorithmFromId(int id) {
    switch (id) {
        case 0: return Algorithm::BFS;
        case 1: return Algorithm::DFS;
        default: return Algorithm::ASTAR;
    }
}

const char *algorithmName(Algorithm algo) {
    switch (algo) {
        case Algorithm::BFS:   return "BFS";
        case Algorithm::DFS:   return "DFS";
        case Algorithm::ASTAR: return "ASTAR";
    }
    return "?";
}

void resolveAndResetAnimation() {
    result = solve(*maze, currentAlgo);
    animatedStep = 0;
    stepTimer = 0.0f;
    animationDone = false;
}

// Salva o labirinto atual (uma unica vez, se o arquivo ainda nao existir)
// e ACRESCENTA um bloco de estatisticas do algoritmo atual ao final do
// arquivo. Assim, rodando BFS/DFS/A* no mesmo labirinto e salvando no
// mesmo arquivo, todas as estatisticas ficam acumuladas para comparacao
// posterior (ex.: gerar um grafico).
void saveMazeAndStatsToFile(const std::string &path) {
    if (!maze) {
        std::cerr << "Nenhum labirinto ativo para salvar." << std::endl;
        return;
    }

    bool fileExists = std::ifstream(path).good();

    std::ofstream out(path, std::ios::app);
    if (!out.is_open()) {
        std::cerr << "Nao foi possivel abrir o arquivo para salvar: " << path << std::endl;
        return;
    }

    if (!fileExists) {
        out << "# LABIRINTO\n";
        out << "largura=" << maze->width() << "\n";
        out << "altura=" << maze->height() << "\n";
        out << "# cada celula: 4 digitos NESW (1 = tem parede, 0 = passagem)\n";
        out << "celulas=\n";
        for (int y = 0; y < maze->height(); ++y) {
            for (int x = 0; x < maze->width(); ++x) {
                const Cell &c = maze->at(x, y);
                out << (c.walls[NORTH] ? 1 : 0)
                    << (c.walls[EAST]  ? 1 : 0)
                    << (c.walls[SOUTH] ? 1 : 0)
                    << (c.walls[WEST]  ? 1 : 0);
                if (x != maze->width() - 1) out << " ";
            }
            out << "\n";
        }
        out << "\n";
    }

    out << "# ESTATISTICAS\n";
    out << "algoritmo=" << algorithmName(currentAlgo) << "\n";
    out << "custo=" << result.cost << "\n";
    out << "nos_expandidos=" << result.nodesExpanded << "\n";
    out << "max_nos_memoria=" << result.maxNodesInMemory << "\n";
    out << "iteracoes=" << result.iterations << "\n";
    out << "\n";

    out.close();
    std::cout << "Labirinto/estatisticas salvos em: " << path << std::endl;
}

void framebuffer_size_callback(GLFWwindow *, int width, int height) {
    glViewport(0, 0, width, height);
}

} // namespace

// ---------------------------------------------------------------------
// Java_main_MotorGrafico_init
// Cria a janela OpenGL, o labirinto e o renderer, e entra no loop de
// renderizacao. So retorna quando a janela e fechada (windowShouldClose).
// ---------------------------------------------------------------------
JNIEXPORT void JNICALL Java_main_MotorGrafico_init
  (JNIEnv *, jobject, jint mazeWidth, jint mazeHeight) {

    if (!glfwInit()) {
        std::cerr << "Falha ao inicializar o GLFW" << std::endl;
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const int screenWidth = 900;
    const int screenHeight = 700;

    window = glfwCreateWindow(screenWidth, screenHeight,
                               "Simulador de Labirinto - Java + OpenGL (JNI)",
                               nullptr, nullptr);
    if (!window) {
        std::cerr << "Falha ao criar a janela GLFW" << std::endl;
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Falha ao inicializar o GLAD" << std::endl;
        return;
    }

    maze = new Maze(mazeWidth, mazeHeight);
    maze->generate();

    renderer = new Renderer(screenWidth, screenHeight);
    renderer->computeCellSize(*maze);
    renderer->initOpenGLResources();

    currentAlgo = Algorithm::BFS;
    resolveAndResetAnimation();

    float lastFrameTime = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window) && !g_shouldClose.load()) {

        float currentFrameTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        // --- processa solicitacoes vindas da interface Swing ---
        if (g_requestNewMaze.exchange(false)) {
            maze->generate();
            renderer->computeCellSize(*maze);
            resolveAndResetAnimation();
        }

        int reqAlgo = g_requestAlgorithm.exchange(-1);
        if (reqAlgo != -1) {
            currentAlgo = algorithmFromId(reqAlgo);
            resolveAndResetAnimation();
        }

        if (g_requestReset.exchange(false)) {
            animatedStep = 0;
            stepTimer = 0.0f;
            animationDone = false;
        }

        if (g_requestSave.exchange(false)) {
            std::string path;
            {
                std::lock_guard<std::mutex> lock(g_savePathMutex);
                path = g_savePath;
            }
            saveMazeAndStatsToFile(path);
        }

        renderer->cameraYaw = g_cameraYaw.load();
        renderer->cameraPitch = g_cameraPitch.load();

        // --- atualiza a animacao (mesmo esquema do main.cpp original) ---
        size_t totalAnimationSteps = result.visitOrder.size() + result.path.size();
        float interval = g_stepInterval.load();

        // VOLTAR um passo (ocorre instantaneamente)
        if (g_requestStepBack.exchange(false)) {
            if (animatedStep > 0) {
                animatedStep--;
                animationDone = false; 
            }
        }

        // AVANÇAR e TOCAR (só roda se não estiver pausado)
        if (!g_isPaused.load() && !animationDone) {
            stepTimer += deltaTime;
            while (stepTimer >= interval && animatedStep < totalAnimationSteps) {
                animatedStep++;
                stepTimer -= interval;
            }
            if (animatedStep >= totalAnimationSteps) {
                animationDone = true;
            }
        }

        // --- desenho ---
        glClearColor(0.2f, 0.2f, 0.25f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderer->drawVisited(*maze, result, animatedStep);
        renderer->drawPath(*maze, result, animatedStep);
        renderer->drawStartAndGoal(*maze);
        renderer->drawMaze(*maze);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // limpeza final caso a janela tenha sido fechada pelo X e nao via cleanup()
    delete renderer;
    delete maze;
    renderer = nullptr;
    maze = nullptr;

    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}

JNIEXPORT void JNICALL Java_main_MotorGrafico_generateNewMaze
  (JNIEnv *, jobject) {
    g_requestNewMaze.store(true);
}

JNIEXPORT void JNICALL Java_main_MotorGrafico_setAlgorithm
  (JNIEnv *, jobject, jint algorithmId) {
    g_requestAlgorithm.store(algorithmId);
}

JNIEXPORT void JNICALL Java_main_MotorGrafico_resetAnimation
  (JNIEnv *, jobject) {
    g_requestReset.store(true);
}

JNIEXPORT void JNICALL Java_main_MotorGrafico_setAnimationSpeed
  (JNIEnv *, jobject, jfloat stepIntervalSeconds) {
    if (stepIntervalSeconds < 0.001f) stepIntervalSeconds = 0.001f;
    g_stepInterval.store(stepIntervalSeconds);
}

JNIEXPORT void JNICALL Java_main_MotorGrafico_setCamera
  (JNIEnv *, jobject, jfloat yaw, jfloat pitch) {
    if (pitch < 1.0f) pitch = 1.0f;
    if (pitch > 89.0f) pitch = 89.0f;
    g_cameraYaw.store(yaw);
    g_cameraPitch.store(pitch);
}

JNIEXPORT void JNICALL Java_main_MotorGrafico_saveMaze
  (JNIEnv *env, jobject, jstring filePath) {
    const char *pathChars = env->GetStringUTFChars(filePath, nullptr);
    {
        std::lock_guard<std::mutex> lock(g_savePathMutex);
        g_savePath = pathChars;
    }
    env->ReleaseStringUTFChars(filePath, pathChars);
    g_requestSave.store(true);
}

JNIEXPORT void JNICALL Java_main_MotorGrafico_cleanup
  (JNIEnv *, jobject) {
    g_shouldClose.store(true);
    if (window) {
        glfwSetWindowShouldClose(window, true);
    }
  }

JNIEXPORT void JNICALL Java_main_MotorGrafico_setPaused
  (JNIEnv *, jobject, jboolean paused) {
    g_isPaused.store(paused);
}

JNIEXPORT void JNICALL Java_main_MotorGrafico_stepBack
  (JNIEnv *, jobject) {
    g_requestStepBack.store(true);
}