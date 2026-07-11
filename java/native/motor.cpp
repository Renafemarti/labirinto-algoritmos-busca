#include <jni.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <array>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

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

const int SCREEN_WIDTH = 900;
const int SCREEN_HEIGHT = 700;

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
std::atomic<bool> g_isPaused{false};
std::atomic<bool> g_requestStepBack{false};

// -----------------------------------------------------------------
// Estatisticas de todos os algoritmos (para o grafico de barras).
// Segue o mesmo padrao de "pedido + processamento no loop de render"
// usado por g_requestSave: a thread Swing (EDT) seta g_requestStats,
// a OpenGL-Thread calcula e guarda o resultado em g_statsResult, e a
// EDT fica esperando g_statsReady virar true antes de ler o texto.
// -----------------------------------------------------------------
std::atomic<bool> g_requestStats{false};
std::atomic<bool> g_statsReady{false};
std::mutex g_statsMutex;
std::string g_statsResult;

Algorithm algorithmFromId(int id) {
    switch (id) {
        case 0: return Algorithm::BFS;
        case 1: return Algorithm::DFS;
        case 2: return Algorithm::ASTAR;
        case 3: return Algorithm::DIJKSTRA;
        case 4: return Algorithm::GREEDY;
        default: return Algorithm::ASTAR;
    }
}

const char *algorithmName(Algorithm algo) {
    switch (algo) {
        case Algorithm::BFS:      return "BFS";
        case Algorithm::DFS:      return "DFS";
        case Algorithm::ASTAR:    return "ASTAR";
        case Algorithm::DIJKSTRA: return "DIJKSTRA";
        case Algorithm::GREEDY:   return "GREEDY";
    }
    return "?";
}

void resolveAndResetAnimation() {
    result = solve(*maze, currentAlgo);
    animatedStep = 0;
    stepTimer = 0.0f;
    animationDone = false;
}

// -----------------------------------------------------------------
// Parser/validador do arquivo salvo por saveMazeAndStatsToFile().
// Le apenas o bloco "# LABIRINTO" (ignora as "# ESTATISTICAS" que
// vierem depois). Retorna false se o formato nao bater exatamente
// com o esperado, sem lancar excecao e sem travar o programa.
// -----------------------------------------------------------------
bool loadMazeFromFile(const std::string &path, int &outWidth, int &outHeight,
                       std::vector<std::array<bool, 4>> &outWalls) {
    std::ifstream in(path);
    if (!in.is_open()) return false;

    std::string line;
    bool foundHeader = false;
    bool foundCelulas = false;
    int width = -1;
    int height = -1;

    while (std::getline(in, line)) {
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue; // linha em branco
        line = line.substr(start);

        if (!foundHeader) {
            if (line.rfind("# LABIRINTO", 0) == 0) {
                foundHeader = true;
            } else {
                return false; // primeiro conteudo relevante nao e o header esperado
            }
            continue;
        }

        if (line.rfind("largura=", 0) == 0) {
            try { width = std::stoi(line.substr(8)); } catch (...) { return false; }
            continue;
        }
        if (line.rfind("altura=", 0) == 0) {
            try { height = std::stoi(line.substr(7)); } catch (...) { return false; }
            continue;
        }
        if (line.rfind("#", 0) == 0) continue; // linha de comentario
        if (line.rfind("celulas=", 0) == 0) {
            foundCelulas = true;
            break;
        }

        return false; // linha inesperada dentro do cabecalho
    }

    if (!foundHeader || !foundCelulas || width <= 0 || height <= 0) return false;

    outWidth = width;
    outHeight = height;
    outWalls.assign(static_cast<size_t>(width) * height, {true, true, true, true});

    for (int y = 0; y < height; ++y) {
        if (!std::getline(in, line)) return false;

        std::istringstream iss(line);
        std::string token;
        int x = 0;
        while (iss >> token) {
            if (x >= width || token.size() != 4) return false;
            for (int i = 0; i < 4; ++i) {
                if (token[i] != '0' && token[i] != '1') return false;
                outWalls[static_cast<size_t>(y) * width + x][i] = (token[i] == '1');
            }
            ++x;
        }
        if (x != width) return false;
    }

    return true;
}

// Salva o labirinto atual (uma unica vez, se o arquivo ainda nao existir)
// e ACRESCENTA um bloco de estatisticas do algoritmo atual ao final do arquivo
void saveMazeAndStatsToFile(const std::string &path) {
    if (!maze) {
        std::cerr << "Nenhum labirinto ativo para salvar." << std::endl;
        return;
    }
 
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) {
        std::cerr << "Nao foi possivel abrir o arquivo para salvar: " << path << std::endl;
        return;
    }
 
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
 
    static const Algorithm todosAlgoritmos[] = {
        Algorithm::BFS, Algorithm::DFS, Algorithm::ASTAR,
        Algorithm::DIJKSTRA, Algorithm::GREEDY
    };
 
    for (Algorithm algo : todosAlgoritmos) {
        // Resolve o labirinto atual com este algoritmo especifico,
        // independente do que esta selecionado/animado na tela agora.
        SolveResult r = solve(*maze, algo);
 
        out << "# ESTATISTICAS\n";
        out << "algoritmo=" << algorithmName(algo) << "\n";
        out << "custo=" << r.cost << "\n";
        out << "nos_expandidos=" << r.nodesExpanded << "\n";
        out << "max_nos_memoria=" << r.maxNodesInMemory << "\n";
        out << "iteracoes=" << r.iterations << "\n";
        out << "\n";
    }
 
    out.close();
    std::cout << "Labirinto e estatisticas de todos os algoritmos salvos em: " << path << std::endl;
}


// -----------------------------------------------------------------
// Roda os 5 algoritmos no labirinto atual e monta uma string com uma
// linha por algoritmo, no formato:
//   nome;custo;nos_expandidos;max_nos_memoria;iteracoes
// E' esse texto que a EstatisticasUI (Java) faz o parse para desenhar
// o grafico de barras. Nao mexe no 'result'/'currentAlgo' que estao
// sendo animados na tela.
// -----------------------------------------------------------------
std::string computeStatisticsText() {
    if (!maze) return "";

    static const Algorithm todosAlgoritmos[] = {
        Algorithm::BFS, Algorithm::DFS, Algorithm::ASTAR,
        Algorithm::DIJKSTRA, Algorithm::GREEDY
    };

    std::ostringstream out;
    for (Algorithm algo : todosAlgoritmos) {
        SolveResult r = solve(*maze, algo);
        out << algorithmName(algo) << ";"
            << r.cost << ";"
            << r.nodesExpanded << ";"
            << r.maxNodesInMemory << ";"
            << r.iterations << "\n";
    }
    return out.str();
}

void framebuffer_size_callback(GLFWwindow *, int width, int height) {
    glViewport(0, 0, width, height);
}

// -----------------------------------------------------------------
// Cria a janela GLFW e inicializa o GLAD. Retorna false em caso de
// falha (e ja imprime o motivo no stderr).
// -----------------------------------------------------------------
bool setupWindowAndGL() {
    if (!glfwInit()) {
        std::cerr << "Falha ao inicializar o GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT,
                               "Simulador de Labirinto - Java + OpenGL (JNI)",
                               nullptr, nullptr);
    if (!window) {
        std::cerr << "Falha ao criar a janela GLFW" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Falha ao inicializar o GLAD" << std::endl;
        return false;
    }

    return true;
}

// -----------------------------------------------------------------
// Laco principal de renderizacao. So retorna quando a janela fecha
// (pelo X ou por cleanup()). Compartilhado entre init() e initFromFile().
// -----------------------------------------------------------------
void runRenderLoop() {
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

        if (g_requestStats.exchange(false)) {
            std::string text = computeStatisticsText();
            {
                std::lock_guard<std::mutex> lock(g_statsMutex);
                g_statsResult = text;
            }
            g_statsReady.store(true);
        }

        renderer->cameraYaw = g_cameraYaw.load();
        renderer->cameraPitch = g_cameraPitch.load();

        // --- atualiza a animacao ---
        size_t totalAnimationSteps = result.visitOrder.size() + result.path.size();
        float interval = g_stepInterval.load();
        // pause e volta
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
}

void teardown() {
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

} // namespace

// ---------------------------------------------------------------------
// Java_main_MotorGrafico_init
// Cria um labirinto NOVO (gerado aleatoriamente) com o tamanho pedido.
// ---------------------------------------------------------------------
JNIEXPORT void JNICALL Java_main_MotorGrafico_init
  (JNIEnv *, jobject, jint mazeWidth, jint mazeHeight) {

    if (!setupWindowAndGL()) return;

    maze = new Maze(mazeWidth, mazeHeight);
    maze->generate();

    renderer = new Renderer(SCREEN_WIDTH, SCREEN_HEIGHT);
    renderer->computeCellSize(*maze);
    renderer->initOpenGLResources();

    currentAlgo = Algorithm::BFS;
    resolveAndResetAnimation();

    runRenderLoop();
    teardown();
}

// ---------------------------------------------------------------------
// Java_main_MotorGrafico_validateMazeFile
// So faz a leitura/validacao do arquivo (rapido, nao abre janela).
// Usado pelo botao "Abrir Labirinto Salvo" antes de decidir se abre a
// tela de controles.
// ---------------------------------------------------------------------
JNIEXPORT jboolean JNICALL Java_main_MotorGrafico_validateMazeFile
  (JNIEnv *env, jobject, jstring filePath) {

    const char *pathChars = env->GetStringUTFChars(filePath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(filePath, pathChars);

    int width, height;
    std::vector<std::array<bool, 4>> walls;
    bool ok = loadMazeFromFile(path, width, height, walls);
    return ok ? JNI_TRUE : JNI_FALSE;
}

// ---------------------------------------------------------------------
// Java_main_MotorGrafico_initFromFile
// Carrega o labirinto EXATAMENTE como foi salvo (sem chamar generate())
// e abre a janela OpenGL com ele. So deve ser chamado depois que
// validateMazeFile() ja confirmou que o arquivo e valido.
// ---------------------------------------------------------------------
JNIEXPORT void JNICALL Java_main_MotorGrafico_initFromFile
  (JNIEnv *env, jobject, jstring filePath) {

    const char *pathChars = env->GetStringUTFChars(filePath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(filePath, pathChars);

    int width, height;
    std::vector<std::array<bool, 4>> walls;
    if (!loadMazeFromFile(path, width, height, walls)) {
        std::cerr << "Arquivo de labirinto invalido (initFromFile): " << path << std::endl;
        return;
    }

    if (!setupWindowAndGL()) return;

    maze = new Maze(width, height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Cell &c = maze->at(x, y);
            const auto &w = walls[static_cast<size_t>(y) * width + x];
            c.walls[NORTH] = w[0];
            c.walls[EAST]  = w[1];
            c.walls[SOUTH] = w[2];
            c.walls[WEST]  = w[3];
        }
    }

    renderer = new Renderer(SCREEN_WIDTH, SCREEN_HEIGHT);
    renderer->computeCellSize(*maze);
    renderer->initOpenGLResources();

    currentAlgo = Algorithm::BFS;
    resolveAndResetAnimation();

    runRenderLoop();
    teardown();
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

// ---------------------------------------------------------------------
// Java_main_MotorGrafico_getStatistics
// Pede para a OpenGL-Thread calcular as estatisticas dos 5 algoritmos
// no labirinto atual e espera (bloqueando a thread que chamou, em
// geral a EDT do Swing) ate o resultado ficar pronto. Tem um limite
// de espera de ~1s para nao travar a interface caso a janela OpenGL
// nao esteja rodando (ex.: motor.init() ainda nao terminou).
// ---------------------------------------------------------------------
JNIEXPORT jstring JNICALL Java_main_MotorGrafico_getStatistics
  (JNIEnv *env, jobject) {

    if (!maze) {
        return env->NewStringUTF("");
    }

    g_statsReady.store(false);
    g_requestStats.store(true);

    const int maxWaitMs = 1000;
    int waitedMs = 0;
    while (!g_statsReady.load() && waitedMs < maxWaitMs) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        waitedMs += 5;
    }

    std::string text;
    {
        std::lock_guard<std::mutex> lock(g_statsMutex);
        text = g_statsResult;
    }
    return env->NewStringUTF(text.c_str());
}