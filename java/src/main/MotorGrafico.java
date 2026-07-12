package main;

import java.awt.Canvas;

public class MotorGrafico {

    static {
        System.loadLibrary("motor");
    }

    // Inicia GLFW + GLAD, cria o labirinto, o renderer e entra no loop de
    // renderização. Deve ser chamado em uma thread separada da EDT.
    //
    // 'canvas' e o java.awt.Canvas (heavyweight) que fica dentro do JPanel
    // da direita em TransformUI. O lado nativo usa JAWT para pegar a Window
    // X11 por tras desse Canvas e faz a janela GLFW nascer "reparentada"
    // dentro dela (ver embedIntoCanvas() em motor.cpp) — e assim que a
    // janela do labirinto e a janela de controles viram uma unica janela.
    // O Canvas PRECISA ja estar displayable (addNotify() chamado, ou seja,
    // a janela Swing ja visivel) antes de chamar init()/initFromFile().
    public native void init(Canvas canvas, int mazeWidth, int mazeHeight);

    // Verifica (rapidamente, sem abrir janela) se o arquivo indicado
    // esta no formato correto de labirinto salvo (gerado por saveMaze).
    // Retorna true se for valido.
    public native boolean validateMazeFile(String filePath);

    // Inicia GLFW + GLAD e carrega o labirinto EXATAMENTE como esta no
    // arquivo indicado (sem gerar um novo). So deve ser chamado depois
    // que validateMazeFile() retornou true. Deve ser chamado em uma
    // thread separada da EDT, assim como init(). Mesma observacao sobre
    // o Canvas precisar estar displayable.
    public native void initFromFile(Canvas canvas, String filePath);

    // Avisa o lado nativo que o Canvas mudou de tamanho (o usuario
    // redimensionou a janela combinada). Redimensiona a janela GLFW
    // embutida para bater com o novo tamanho do Canvas e ajusta o
    // viewport/aspect ratio da projecao 3D. Seguro para chamar da EDT a
    // qualquer momento (so seta um pedido atomico, processado no proximo
    // frame do loop de render, igual aos outros comandos).
    public native void resizeViewport(int width, int height);

    // Gera um novo labirinto (novo layout) e reseta a animação/solução.
    // Usa o algoritmo de GERACAO selecionado por setMazeAlgorithm().
    public native void generateNewMaze();

    // Seleciona o algoritmo de GERACAO do labirinto: 0 = Recursive
    // Backtracking, 1 = Prim, 2 = Kruskal. So tem efeito no proximo
    // labirinto criado (init() ou generateNewMaze()) — nao regenera o
    // labirinto atual na hora, diferente de setAlgorithm() (busca).
    public native void setMazeAlgorithm(int mazeAlgorithmId);

    // Seleciona o algoritmo de busca: 0 = BFS, 1 = DFS, 2 = A*.
    // Já resolve o labirinto novamente e reinicia a animação.
    public native void setAlgorithm(int algorithmId);

    // Reinicia a animação (do zero) mantendo o mesmo labirinto e algoritmo.
    public native void resetAnimation();

    // Define o intervalo (em segundos) entre cada passo da animação.
    // Quanto menor, mais rápida a animação.
    public native void setAnimationSpeed(float stepIntervalSeconds);

    // Define os ângulos absolutos da câmera orbital (em graus).
    // Mantido por compatibilidade (ex.: reset de câmera), mas o controle
    // principal agora é feito arrastando o mouse na própria janela do
    // labirinto (ver TransformUI / callbacks de mouse em motor.cpp).
    public native void setCamera(float yaw, float pitch);

    // Salva o labirinto atual e as estatísticas do último algoritmo
    // executado (custo, nós expandidos, máximo de nós em memória,
    // iterações) no arquivo indicado. Se o arquivo já existir, apenas
    // acrescenta um novo bloco de estatísticas (útil para comparar
    // vários algoritmos no mesmo labirinto).
    public native void saveMaze(String filePath);

    // Libera os recursos do OpenGL e fecha a janela.
    public native void cleanup();

    public native void setPaused(boolean paused);
    public native void stepBack();
    public native String getStatistics();
}