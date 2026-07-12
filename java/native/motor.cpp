#include <jni.h>
#include <glad/glad.h>

#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

// JAWT (Java AWT Native Interface): usado para pegar a Window X11 nativa
// por tras do java.awt.Canvas mostrado em TransformUI, para podermos
// "reparentar" a janela GLFW para dentro dela (ver embedIntoCanvas()).
#include <jawt.h>
#include <jawt_md.h>
#include <X11/Xlib.h>

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

// Usados apenas como fallback caso, por algum motivo, o JAWT nao consiga
// ler o tamanho atual do Canvas (ver embedIntoCanvas()). Na pratica a
// janela nasce com o tamanho real do Canvas na hora do embed, e depois
// e redimensionada dinamicamente via resizeViewport()/g_requestResize.
const int SCREEN_WIDTH = 900;
const int SCREEN_HEIGHT = 700;

GLFWwindow *window = nullptr;
Maze *maze = nullptr;
Renderer *renderer = nullptr;

// Guarda o Display X11 usado pelo GLFW, para as chamadas de reparenting/
// resize (XReparentWindow/XResizeWindow) feitas em embedIntoCanvas() e
// no processamento de g_requestResize dentro de runRenderLoop().
Display *g_x11Display = nullptr;

// Tamanho real (em pixels) do Canvas no momento do embed, lido via JAWT
// em embedIntoCanvas(). So usado nesta mesma thread (OpenGL-Thread), logo
// apos o embed, para construir o Renderer com o aspect ratio certo — por
// isso nao precisa ser atomic.
int g_embedWidth = SCREEN_WIDTH;
int g_embedHeight = SCREEN_HEIGHT;

// -----------------------------------------------------------------
// Controle da câmera via mouse (arrastar com o botão esquerdo orbita,
// a roda do mouse aproxima/afasta). Os callbacks do GLFW rodam na
// mesma thread do loop de render (thread "OpenGL-Thread"), entao nao
// ha problema de concorrencia aqui: escrevemos direto nos mesmos
// atomics que a UI Swing usa (g_cameraYaw/g_cameraPitch).
// -----------------------------------------------------------------
bool g_leftMouseDown = false;
double g_lastMouseX = 0.0;
double g_lastMouseY = 0.0;
std::atomic<float> g_cameraZoom{1.0f};

// -----------------------------------------------------------------
// Redimensionamento vindo do Canvas Swing/AWT (TransformUI). A janela
// GLFW agora e uma janela FILHA (reparentada via X11) do java.awt.Canvas
// mostrado na TransformUI, entao nao existe mais "posicao" para
// sincronizar (o gerenciador de janelas move a janela toda de uma vez,
// como uma unica janela) — so o TAMANHO do Canvas muda quando o usuario
// redimensiona/maximiza a janela, e por isso precisamos reagir a isso.
// A EDT chama Java_main_MotorGrafico_resizeViewport sempre que o Canvas
// muda de tamanho; aqui so guardamos o pedido e aplicamos no inicio do
// proximo frame do loop de render (glfwSetWindowSize so deve ser chamado
// na thread que criou a janela GLFW).
// -----------------------------------------------------------------
std::atomic<bool> g_requestResize{false};
std::atomic<int> g_resizeWidth{0};
std::atomic<int> g_resizeHeight{0};

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

// Algoritmo de GERACAO do labirinto usado na proxima vez que um labirinto
// novo for criado (init() ou "Novo Labirinto"/generateNewMaze()). Guardado
// como int (mesma convencao usada para os algoritmos de busca): 0 =
// Recursive Backtracking, 1 = Prim, 2 = Kruskal. Nao afeta labirintos
// carregados de arquivo (initFromFile), que ja vem prontos.
std::atomic<int> g_mazeAlgorithm{0};

std::atomic<int> g_requestAlgorithm{-1}; // -1 = nenhuma solicitacao pendente
std::atomic<bool> g_shouldClose{false};

// Fica 'true' do inicio de runRenderLoop() ate o final de teardown().
// Java_main_MotorGrafico_cleanup espera esta flag virar 'false' antes de
// devolver o controle para quem chamou: sem isso, era possivel abrir um
// SEGUNDO MotorGrafico (voltar ao menu -> gerar novo labirinto) enquanto
// a thread OpenGL antiga ainda estava destruindo a janela GLFW/X11,
// causando corrupcao de memoria (SIGSEGV / "corrupted double-linked list").
std::atomic<bool> g_engineActive{false};

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

MazeAlgorithm mazeAlgorithmFromId(int id) {
    switch (id) {
        case 0: return MazeAlgorithm::RECURSIVE_BACKTRACKING;
        case 1: return MazeAlgorithm::PRIM;
        case 2: return MazeAlgorithm::KRUSKAL;
        default: return MazeAlgorithm::RECURSIVE_BACKTRACKING;
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
// Zera todas as flags/solicitacoes pendentes antes de comecar um novo
// motor (init()/initFromFile()). Necessario porque essas variaveis sao
// globais do processo/da .so — sem resetar, um MotorGrafico criado
// depois de "Voltar ao Menu" herdaria o estado (ex: g_shouldClose ainda
// 'true') deixado pela instancia anterior e fecharia a janela sozinho
// ou se comportaria de forma inconsistente.
// -----------------------------------------------------------------
void resetEngineState() {
    g_shouldClose.store(false);
    g_requestNewMaze.store(false);
    g_requestReset.store(false);
    g_requestAlgorithm.store(-1);
    g_requestSave.store(false);
    g_requestStats.store(false);
    g_statsReady.store(false);
    g_requestStepBack.store(false);
    g_isPaused.store(false);
    g_cameraZoom.store(1.0f);
    g_cameraYaw.store(45.0f);
    g_cameraPitch.store(45.0f);
    g_requestResize.store(false);
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
        out << "tempo_ms=" << r.executionTimeMs << "\n";
        out << "iteracoes=" << r.iterations << "\n";
        out << "max_memoria=" << r.maxNodesInMemory << "\n";
        out << "\n";
    }
 
    out.close();
    std::cout << "Labirinto e estatisticas de todos os algoritmos salvos em: " << path << std::endl;
}


// -----------------------------------------------------------------
// Roda os 5 algoritmos no labirinto atual e monta uma string com uma
// linha por algoritmo, no formato:
//   nome;custo;nos_expandidos;tempo_ms;iteracoes;max_memoria
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
            << r.executionTimeMs << ";"
            << r.iterations << ";"
            << r.maxNodesInMemory << "\n";
    }
    return out.str();
}

void framebuffer_size_callback(GLFWwindow *, int width, int height) {
    glViewport(0, 0, width, height);
}

// -----------------------------------------------------------------
// Callbacks de mouse: arrastar com o botao esquerdo orbita a camera
// (yaw/pitch), a roda do mouse aproxima/afasta (zoom). Substituem os
// antigos sliders de "Rotacao"/"Inclinacao" da UI Swing.
// -----------------------------------------------------------------
void mouse_button_callback(GLFWwindow *win, int button, int action, int) {
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;

    if (action == GLFW_PRESS) {
        g_leftMouseDown = true;
        glfwGetCursorPos(win, &g_lastMouseX, &g_lastMouseY);
    } else if (action == GLFW_RELEASE) {
        g_leftMouseDown = false;
    }
}

void cursor_pos_callback(GLFWwindow *, double xpos, double ypos) {
    if (g_leftMouseDown) {
        double dx = xpos - g_lastMouseX;
        double dy = ypos - g_lastMouseY;

        const float sensitivity = 0.3f;
        float yaw = g_cameraYaw.load() + static_cast<float>(dx) * sensitivity;
        float pitch = g_cameraPitch.load() - static_cast<float>(dy) * sensitivity;

        while (yaw < 0.0f) yaw += 360.0f;
        while (yaw >= 360.0f) yaw -= 360.0f;
        if (pitch < 1.0f) pitch = 1.0f;
        if (pitch > 89.0f) pitch = 89.0f;

        g_cameraYaw.store(yaw);
        g_cameraPitch.store(pitch);
    }
    g_lastMouseX = xpos;
    g_lastMouseY = ypos;
}

void scroll_callback(GLFWwindow *, double, double yoffset) {
    const float zoomStep = 0.06f;
    float zoom = g_cameraZoom.load() - static_cast<float>(yoffset) * zoomStep;
    if (zoom < 0.3f) zoom = 0.3f;
    if (zoom > 3.0f) zoom = 3.0f;
    g_cameraZoom.store(zoom);
}

// -----------------------------------------------------------------
// Le, via JAWT, a Window X11 nativa por tras de um java.awt.Canvas e o
// tamanho atual dele (em pixels). Retorna a Window (ou 0 em caso de
// falha) e preenche outWidth/outHeight.
//
// PRE-REQUISITO: o Canvas precisa estar "displayable" (a janela Swing
// que o contem ja precisa ter sido mostrada com setVisible(true) e o
// addNotify() do AWT ja ter rodado) — ver o comentario em MotorGrafico
// .init()/.initFromFile() e o fluxo em LauncherUI.
// -----------------------------------------------------------------
Window getX11WindowFromCanvas(JNIEnv *env, jobject canvasObj, int &outWidth, int &outHeight) {
    JAWT awt;
    awt.version = JAWT_VERSION_9;
    if (JAWT_GetAWT(env, &awt) == JNI_FALSE) {
        std::cerr << "JAWT_GetAWT falhou (JVM sem suporte a JAWT?)" << std::endl;
        return 0;
    }

    JAWT_DrawingSurface *ds = awt.GetDrawingSurface(env, canvasObj);
    if (!ds) {
        std::cerr << "JAWT GetDrawingSurface falhou (Canvas nulo?)" << std::endl;
        return 0;
    }

    jint lock = ds->Lock(ds);
    if ((lock & JAWT_LOCK_ERROR) != 0) {
        std::cerr << "JAWT Lock falhou (Canvas ainda nao esta displayable?)" << std::endl;
        awt.FreeDrawingSurface(ds);
        return 0;
    }

    Window resultWindow = 0;
    JAWT_DrawingSurfaceInfo *dsi = ds->GetDrawingSurfaceInfo(ds);
    if (dsi) {
        auto *dsiX11 = reinterpret_cast<JAWT_X11DrawingSurfaceInfo *>(dsi->platformInfo);
        if (dsiX11) {
            resultWindow = dsiX11->drawable;
            outWidth = dsi->bounds.width;
            outHeight = dsi->bounds.height;
        }
        ds->FreeDrawingSurfaceInfo(dsi);
    }

    ds->Unlock(ds);
    awt.FreeDrawingSurface(ds);
    return resultWindow;
}

// -----------------------------------------------------------------
// Cria a janela GLFW, inicializa o GLAD, e REPARENTA a janela GLFW para
// dentro da Window X11 do Canvas Swing (embedding via X11, a mesma
// tecnica usada para embutir players de video/plugins dentro de janelas
// Java). A partir daqui, do ponto de vista do gerenciador de janelas do
// SO, existe UMA UNICA janela (a JFrame da TransformUI): move-la ou
// redimensiona-la move/redimensiona o labirinto junto.
//
// Retorna false em caso de falha (e ja imprime o motivo no stderr).
// -----------------------------------------------------------------
bool embedIntoCanvas(JNIEnv *env, jobject canvasObj) {
    int canvasWidth = SCREEN_WIDTH;
    int canvasHeight = SCREEN_HEIGHT;
    Window parentWindow = getX11WindowFromCanvas(env, canvasObj, canvasWidth, canvasHeight);
    if (parentWindow == 0) {
        std::cerr << "Nao foi possivel obter a Window X11 do Canvas via JAWT." << std::endl;
        return false;
    }
    if (canvasWidth <= 0) canvasWidth = SCREEN_WIDTH;
    if (canvasHeight <= 0) canvasHeight = SCREEN_HEIGHT;
    g_embedWidth = canvasWidth;
    g_embedHeight = canvasHeight;

    // O truque de embedding usado aqui (XReparentWindow) so existe em X11.
    // O AWT/Swing do OpenJDK no Linux roda sobre X11 (XToolkit) mesmo numa
    // sessao Wayland, via XWayland — mas o GLFW, se deixado escolher
    // sozinho, prefere o backend NATIVO Wayland quando disponivel. Nesse
    // caso glfwGetX11Display()/glfwGetX11Window() abaixo voltam NULL/0, e
    // XReparentWindow(NULL, ...) trava com SIGSEGV. Forcamos X11 aqui para
    // os dois lados (AWT e GLFW) ficarem no mesmo mundo.
#ifdef GLFW_PLATFORM
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif
    unsetenv("WAYLAND_DISPLAY"); // fallback para versoes do GLFW < 3.4

    if (!glfwInit()) {
        std::cerr << "Falha ao inicializar o GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    window = glfwCreateWindow(canvasWidth, canvasHeight,
                               "Simulador de Labirinto - Java + OpenGL (JNI)",
                               nullptr, nullptr);
    if (!window) {
        std::cerr << "Falha ao criar a janela GLFW" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Falha ao inicializar o GLAD" << std::endl;
        return false;
    }

    g_x11Display = glfwGetX11Display();
    Window childWindow = glfwGetX11Window(window);

    XReparentWindow(g_x11Display, childWindow, parentWindow, 0, 0);
    XResizeWindow(g_x11Display, childWindow,
                  static_cast<unsigned int>(canvasWidth),
                  static_cast<unsigned int>(canvasHeight));
    XMapWindow(g_x11Display, childWindow);
    XFlush(g_x11Display);

    glfwShowWindow(window);
    glViewport(0, 0, canvasWidth, canvasHeight);

    return true;
}

// -----------------------------------------------------------------
// Laco principal de renderizacao. So retorna quando a janela fecha
// (pelo X ou por cleanup()). Compartilhado entre init() e initFromFile().
// -----------------------------------------------------------------
void runRenderLoop() {
    g_engineActive.store(true);

    float lastFrameTime = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window) && !g_shouldClose.load()) {

        float currentFrameTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        // Redimensiona a janela GLFW embutida (e o viewport/aspect ratio
        // da projecao 3D) para acompanhar o Canvas Swing, sempre que o
        // usuario redimensiona/maximiza a janela combinada.
        if (g_requestResize.exchange(false)) {
            int w = g_resizeWidth.load();
            int h = g_resizeHeight.load();
            if (w > 0 && h > 0) {
                glfwSetWindowSize(window, w, h);
                if (g_x11Display) {
                    XResizeWindow(g_x11Display, glfwGetX11Window(window),
                                  static_cast<unsigned int>(w), static_cast<unsigned int>(h));
                }
                glViewport(0, 0, w, h);
                renderer->resize(w, h);
            }
        }

        // --- processa solicitacoes vindas da interface Swing ---
        if (g_requestNewMaze.exchange(false)) {
            maze->generate(0, mazeAlgorithmFromId(g_mazeAlgorithm.load()));
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
        renderer->cameraZoom = g_cameraZoom.load();

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
    g_x11Display = nullptr;

    // So agora, com GLFW/X11 totalmente desligados, e seguro para um
    // novo MotorGrafico (novo init()/initFromFile() em outra thread)
    // comecar. Ver Java_main_MotorGrafico_cleanup, que fica esperando
    // esta flag virar 'false'.
    g_engineActive.store(false);
}

} // namespace

// ---------------------------------------------------------------------
// Java_main_MotorGrafico_init
// Cria um labirinto NOVO (gerado aleatoriamente) com o tamanho pedido.
// ---------------------------------------------------------------------
JNIEXPORT void JNICALL Java_main_MotorGrafico_init
  (JNIEnv *env, jobject, jobject canvas, jint mazeWidth, jint mazeHeight) {

    resetEngineState();

    if (!embedIntoCanvas(env, canvas)) return;

    maze = new Maze(mazeWidth, mazeHeight);
    maze->generate(0, mazeAlgorithmFromId(g_mazeAlgorithm.load()));

    renderer = new Renderer(g_embedWidth, g_embedHeight);
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
  (JNIEnv *env, jobject, jobject canvas, jstring filePath) {

    resetEngineState();

    const char *pathChars = env->GetStringUTFChars(filePath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(filePath, pathChars);

    int width, height;
    std::vector<std::array<bool, 4>> walls;
    if (!loadMazeFromFile(path, width, height, walls)) {
        std::cerr << "Arquivo de labirinto invalido (initFromFile): " << path << std::endl;
        return;
    }

    if (!embedIntoCanvas(env, canvas)) return;

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

    renderer = new Renderer(g_embedWidth, g_embedHeight);
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

// ---------------------------------------------------------------------
// Java_main_MotorGrafico_setMazeAlgorithm
// Define qual algoritmo de GERACAO sera usado na proxima vez que um
// labirinto novo for criado (botao "Novo Labirinto" ou o proximo
// init()). Diferente de setAlgorithm() (busca), aqui NAO ha
// regeneracao automatica: o labirinto atual continua na tela ate o
// usuario pedir um novo.
// ---------------------------------------------------------------------
JNIEXPORT void JNICALL Java_main_MotorGrafico_setMazeAlgorithm
  (JNIEnv *, jobject, jint mazeAlgorithmId) {
    g_mazeAlgorithm.store(mazeAlgorithmId);
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

// ---------------------------------------------------------------------
// Java_main_MotorGrafico_resizeViewport
// Chamado pela EDT (TransformUI) sempre que o Canvas Swing/AWT que
// hospeda a janela GLFW embutida muda de tamanho (janela redimensionada
// ou maximizada). So guarda o pedido; o resize de verdade acontece no
// proximo frame do loop de render (thread OpenGL), ver runRenderLoop().
// ---------------------------------------------------------------------
JNIEXPORT void JNICALL Java_main_MotorGrafico_resizeViewport
  (JNIEnv *, jobject, jint width, jint height) {
    g_resizeWidth.store(width);
    g_resizeHeight.store(height);
    g_requestResize.store(true);
}

JNIEXPORT void JNICALL Java_main_MotorGrafico_cleanup
  (JNIEnv *, jobject) {
    g_shouldClose.store(true);
    if (window) {
        glfwSetWindowShouldClose(window, true);
    }

    // Espera a OpenGL-Thread realmente sair do runRenderLoop() e terminar
    // o teardown() (destruir janela GLFW, glfwTerminate(), etc.) antes de
    // devolver o controle para quem chamou cleanup(). Isso e essencial
    // para o fluxo "Voltar ao Menu": sem essa espera, o usuario podia
    // clicar em "Gerar Labirinto" e comecar um SEGUNDO motor (nova thread
    // OpenGL) enquanto o primeiro ainda estava destruindo GLFW/X11 —
    // duas threads mexendo no mesmo estado global do GLFW/X11 ao mesmo
    // tempo, causando corrupcao de memoria (SIGSEGV / "corrupted
    // double-linked list" / crash dentro de libX11).
    while (g_engineActive.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
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