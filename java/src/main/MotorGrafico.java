package main;

public class MotorGrafico {

    static {
        System.loadLibrary("motor");
    }

    // Inicia GLFW + GLAD, cria o labirinto, o renderer e entra no loop de
    // renderização. Deve ser chamado em uma thread separada da EDT.
    public native void init(int mazeWidth, int mazeHeight);

    // Gera um novo labirinto (novo layout) e reseta a animação/solução.
    public native void generateNewMaze();

    // Seleciona o algoritmo de busca: 0 = BFS, 1 = DFS, 2 = A*.
    // Já resolve o labirinto novamente e reinicia a animação.
    public native void setAlgorithm(int algorithmId);

    // Reinicia a animação (do zero) mantendo o mesmo labirinto e algoritmo.
    public native void resetAnimation();

    // Define o intervalo (em segundos) entre cada passo da animação.
    // Quanto menor, mais rápida a animação.
    public native void setAnimationSpeed(float stepIntervalSeconds);

    // Define os ângulos absolutos da câmera orbital (em graus).
    public native void setCamera(float yaw, float pitch);

    // Salva o labirinto atual e as estatísticas do último algoritmo
    // executado (custo, nós expandidos, máximo de nós em memória,
    // iterações) no arquivo indicado. Se o arquivo já existir, apenas
    // acrescenta um novo bloco de estatísticas (útil para comparar
    // vários algoritmos no mesmo labirinto).
    public native void saveMaze(String filePath);

    // Libera os recursos do OpenGL e fecha a janela.
    public native void cleanup();
}