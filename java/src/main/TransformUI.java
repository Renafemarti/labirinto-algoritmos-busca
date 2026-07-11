package main;

import javax.swing.*;
import javax.swing.event.ChangeEvent;
import java.awt.*;
import java.awt.event.ComponentAdapter;
import java.awt.event.ComponentEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.File;

/**
 * Janela de controles + labirinto, agora como UMA UNICA JANELA de verdade
 * (com barra de titulo normal do sistema operacional).
 *
 * Antes, a janela do labirinto (OpenGL/GLFW, criada em C++) era uma janela
 * nativa totalmente separada da janela Swing, e as duas so ficavam "coladas"
 * porque toda vez que a janela de controles se movia, mandavamos a nova
 * posicao de tela para o lado nativo reposicionar a janela do labirinto
 * (motor.setWindowPosition). Isso funcionava, mas eram duas janelas de
 * verdade (dois donos, dois gerenciadores de janela, sem redimensionamento
 * conjunto).
 *
 * Agora, o labirinto e desenhado dentro de um java.awt.Canvas (mazeCanvas)
 * que fica dentro do JPanel da direita. O lado nativo usa JAWT para pegar o
 * identificador de janela X11 por tras desse Canvas e faz a janela GLFW
 * nascer "reparentada" dentro dela (ver embedIntoCanvas() em motor.cpp).
 * Do ponto de vista do usuario (e do gerenciador de janelas do SO) existe
 * so UMA janela: mover, redimensionar ou maximizar essa JFrame move/redi-
 * mensiona o labirinto junto, automaticamente — sem nenhuma sincronizacao
 * manual de posicao.
 *
 * O controle da camera continua sendo feito arrastando o mouse dentro do
 * Canvas (botao esquerdo = orbitar, scroll = zoom) — ver os callbacks de
 * mouse em motor.cpp, que agora sao registrados na janela GLFW reparentada.
 */
public class TransformUI extends JFrame {

    private static final long serialVersionUID = 1L;

    // Largura do painel de controles e tamanho inicial da janela combinada.
    private static final int CONTROL_WIDTH = 320;
    private static final int INITIAL_MAZE_WIDTH = 900;
    private static final int INITIAL_HEIGHT = 700;

    private final MotorGrafico motor;

    private JSlider sliderSpeed;
    private Canvas mazeCanvas;
    private JComboBox<String> comboAlgoritmoGeracao;

    // Guarda o ultimo tamanho de Canvas avisado ao lado nativo, para nao
    // spammar resizeViewport() com o mesmo valor a cada evento do Swing.
    private int lastCanvasWidth = -1;
    private int lastCanvasHeight = -1;

    public TransformUI(MotorGrafico motor) {
        this.motor = motor;

        setTitle("Simulador de Labirinto");
        setSize(CONTROL_WIDTH + INITIAL_MAZE_WIDTH, INITIAL_HEIGHT);
        setMinimumSize(new Dimension(CONTROL_WIDTH + 300, 400));
        setLocationRelativeTo(null);
        setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);
        setLayout(new BorderLayout());
        getContentPane().setBackground(UITheme.BG);

        add(createControlsPanel(), BorderLayout.WEST);
        add(createMazePanel(), BorderLayout.CENTER);

        addWindowListener(new WindowAdapter() {
            @Override
            public void windowClosing(WindowEvent e) {
                fecharAplicacao();
            }
        });
    }

    /**
     * Cria o JPanel da direita, que serve apenas de "moldura" para o
     * Canvas AWT (heavyweight) onde a janela GLFW e reparentada. O Canvas
     * ocupa 100% do JPanel e acompanha qualquer redimensionamento dele.
     */
    private JPanel createMazePanel() {
        JPanel mazeWrapper = new JPanel(new BorderLayout());
        mazeWrapper.setBackground(UITheme.BG);

        mazeCanvas = new Canvas();
        mazeCanvas.setBackground(UITheme.BG);
        mazeCanvas.setFocusable(true);
        mazeWrapper.add(mazeCanvas, BorderLayout.CENTER);

        // Sempre que o Canvas mudar de tamanho (janela redimensionada,
        // maximizada, etc.), avisa o lado nativo para redimensionar a
        // janela GLFW embutida e recalcular o aspect ratio da projecao.
        mazeCanvas.addComponentListener(new ComponentAdapter() {
            @Override
            public void componentResized(ComponentEvent e) {
                notifyCanvasResized();
            }
        });

        return mazeWrapper;
    }

    private void notifyCanvasResized() {
        int width = mazeCanvas.getWidth();
        int height = mazeCanvas.getHeight();
        if (width <= 0 || height <= 0) return;
        if (width == lastCanvasWidth && height == lastCanvasHeight) return;

        lastCanvasWidth = width;
        lastCanvasHeight = height;

        // O Canvas so fica "displayable" depois que a janela e mostrada;
        // motor.init()/initFromFile() (chamados por LauncherUI logo apos
        // construir esta tela) e que efetivamente criam a janela GLFW e
        // comecam a processar estes pedidos de resize no loop de render.
        motor.resizeViewport(width, height);
    }

    /**
     * Deve ser chamado (pela EDT) DEPOIS de setVisible(true) e depois de
     * iniciar a thread OpenGL, para garantir que o primeiro resize seja
     * mandado assim que o Canvas ja tiver um tamanho real na tela.
     */
    public void syncInitialCanvasSize() {
        notifyCanvasResized();
    }

    public Canvas getMazeCanvas() {
        return mazeCanvas;
    }

    private void fecharAplicacao() {
        motor.cleanup();
        dispose();
        System.exit(0);
    }

    // -----------------------------------------------------------------
    // Painel de controles (esquerda): algoritmo, camera (via mouse) e acoes.
    // -----------------------------------------------------------------
    private JPanel createControlsPanel() {
        JPanel body = new JPanel(new BorderLayout(8, 8));
        body.setPreferredSize(new Dimension(CONTROL_WIDTH, INITIAL_HEIGHT));
        body.setBorder(BorderFactory.createEmptyBorder(8, 8, 8, 8));
        body.setBackground(UITheme.BG);

        body.add(createTopoPanel(), BorderLayout.NORTH);
        body.add(createCameraPanel(), BorderLayout.CENTER);
        body.add(createAcoesPanel(), BorderLayout.SOUTH);
        return body;
    }

    // Empilha o painel de algoritmo de BUSCA com o novo painel de
    // algoritmo de GERACAO do labirinto, um em cima do outro, no topo
    // da coluna de controles. Precisamos limitar a largura maxima de
    // cada painel filho (Integer.MAX_VALUE nao funciona bem com
    // BoxLayout, mas um valor bem grande sim) para que eles esticem
    // horizontalmente ate preencher a coluna, em vez de ficarem com a
    // largura minima/preferida e sobrarem espacos vazios nas laterais.
    private JPanel createTopoPanel() {
        JPanel panel = new JPanel();
        panel.setLayout(new BoxLayout(panel, BoxLayout.Y_AXIS));
        panel.setBackground(UITheme.BG);

        JPanel algoritmoPanel = createAlgoritmoPanel();
        JPanel geracaoPanel = createGeracaoPanel();
        algoritmoPanel.setAlignmentX(Component.LEFT_ALIGNMENT);
        geracaoPanel.setAlignmentX(Component.LEFT_ALIGNMENT);
        algoritmoPanel.setMaximumSize(new Dimension(9999, algoritmoPanel.getPreferredSize().height));
        geracaoPanel.setMaximumSize(new Dimension(9999, geracaoPanel.getPreferredSize().height));

        panel.add(algoritmoPanel);
        panel.add(Box.createVerticalStrut(8));
        panel.add(geracaoPanel);
        return panel;
    }

    private JPanel createAlgoritmoPanel() {
        JPanel panel = new JPanel(new GridLayout(6, 1, 4, 4));
        panel.setBackground(UITheme.BG);
        panel.setBorder(UITheme.titledBorder("Algoritmo de Busca"));

        JButton bfsBtn = UITheme.button("BFS");
        JButton dfsBtn = UITheme.button("DFS");
        JButton astarBtn = UITheme.button("A*");
        JButton dijkstraBtn = UITheme.button("Dijkstra");
        JButton gulosaBtn = UITheme.button("Gulosa (GBS)");

        bfsBtn.addActionListener(e -> motor.setAlgorithm(0));
        dfsBtn.addActionListener(e -> motor.setAlgorithm(1));
        astarBtn.addActionListener(e -> motor.setAlgorithm(2));
        dijkstraBtn.addActionListener(e -> motor.setAlgorithm(3));
        gulosaBtn.addActionListener(e -> motor.setAlgorithm(4));

        JLabel lblEscolha = new JLabel("Escolha o algoritmo:");
        UITheme.styleLabel(lblEscolha);

        panel.add(lblEscolha);
        panel.add(bfsBtn);
        panel.add(dfsBtn);
        panel.add(astarBtn);
        panel.add(dijkstraBtn);
        panel.add(gulosaBtn);

        return panel;
    }

    // Escolhe qual algoritmo de GERACAO sera usado da PROXIMA vez que o
    // labirinto for recriado (botao "Novo Labirinto" logo abaixo, no
    // painel de Acoes). Trocar aqui NAO regenera o labirinto atual na
    // hora — igual ao combo equivalente na tela de lancamento.
    private JPanel createGeracaoPanel() {
        JPanel panel = new JPanel(new BorderLayout(4, 4));
        panel.setBackground(UITheme.BG);
        panel.setBorder(UITheme.titledBorder("Algoritmo de Geracao do Labirinto"));

        comboAlgoritmoGeracao = new JComboBox<>(
                new String[]{"Recursive Backtracking", "Prim", "Kruskal"});
        comboAlgoritmoGeracao.addActionListener(
                e -> motor.setMazeAlgorithm(comboAlgoritmoGeracao.getSelectedIndex()));

        JLabel info = new JLabel("Vale a partir do proximo \"Novo Labirinto\"");
        info.setFont(UITheme.FONT_SMALL);
        info.setForeground(UITheme.TEXT_SECONDARY);

        panel.add(comboAlgoritmoGeracao, BorderLayout.CENTER);
        panel.add(info, BorderLayout.SOUTH);

        return panel;
    }

    private JPanel createCameraPanel() {
        JPanel panel = new JPanel(new BorderLayout(4, 4));
        panel.setBackground(UITheme.BG);
        panel.setBorder(UITheme.titledBorder("Camera e Velocidade"));

        JTextArea instrucoes = new JTextArea(
                "Controle da camera pelo mouse, direto no labirinto:\n\n"
                        + "• Arrastar com o botao esquerdo: orbitar (rotacao/inclinacao)\n"
                        + "• Roda do mouse: aproximar / afastar (zoom)");
        instrucoes.setEditable(false);
        instrucoes.setLineWrap(true);
        instrucoes.setWrapStyleWord(true);
        instrucoes.setOpaque(false);
        instrucoes.setForeground(UITheme.TEXT_SECONDARY);
        instrucoes.setFont(UITheme.FONT_SMALL);

        sliderSpeed = new JSlider(1, 200, 20); // milissegundos (1 a 200ms)
        sliderSpeed.addChangeListener((ChangeEvent e) -> {
            float seconds = sliderSpeed.getValue() / 1000.0f;
            motor.setAnimationSpeed(seconds);
        });
        UITheme.styleSlider(sliderSpeed);

        panel.add(instrucoes, BorderLayout.CENTER);
        panel.add(createSliderPanel("Velocidade da Animacao", sliderSpeed), BorderLayout.SOUTH);
        return panel;
    }

    private JPanel createAcoesPanel() {
        JPanel panel = new JPanel(new GridLayout(7, 1, 4, 4));
        panel.setBackground(UITheme.BG);

        JButton novoLabirintoBtn = UITheme.button("Novo Labirinto");
        novoLabirintoBtn.addActionListener(e -> motor.generateNewMaze());

        JButton reiniciarBtn = UITheme.button("Reiniciar Animacao");
        reiniciarBtn.addActionListener(e -> motor.resetAnimation());

        JButton voltarBtn = UITheme.button("Voltar Passo");
        voltarBtn.addActionListener(e -> motor.stepBack());

        JButton pausarBtn = UITheme.button("Pausar");
        pausarBtn.addActionListener(e -> {
            boolean pausar = pausarBtn.getText().equals("Pausar");
            motor.setPaused(pausar);
            pausarBtn.setText(pausar ? "Continuar" : "Pausar");
        });

        JButton salvarBtn = UITheme.button("Salvar Labirinto e Estatisticas");
        salvarBtn.addActionListener(e -> onSalvarLabirinto());

        JButton estatisticasBtn = UITheme.button("Estatisticas");
        estatisticasBtn.addActionListener(e -> onAbrirEstatisticas());

        JButton menuBtn = UITheme.button("Voltar ao Menu");
        menuBtn.addActionListener(e -> onVoltarAoMenu());

        panel.add(novoLabirintoBtn);
        panel.add(reiniciarBtn);
        panel.add(voltarBtn);
        panel.add(pausarBtn);
        panel.add(salvarBtn);
        panel.add(estatisticasBtn);
        panel.add(menuBtn);

        return panel;
    }

    /**
     * Fecha a janela de controles + labirinto atual (desligando a thread
     * OpenGL/GLFW por baixo, via motor.cleanup()) e abre novamente a tela
     * inicial (LauncherUI), permitindo comecar um novo labirinto do zero
     * sem precisar reiniciar a aplicacao inteira.
     */
    private void onVoltarAoMenu() {
        int confirmacao = JOptionPane.showConfirmDialog(this,
                "Voltar ao menu inicial?\nO labirinto atual e o progresso da animacao serao perdidos.",
                "Voltar ao Menu",
                JOptionPane.YES_NO_OPTION,
                JOptionPane.QUESTION_MESSAGE);

        if (confirmacao != JOptionPane.YES_OPTION) return;

        motor.cleanup();
        dispose();

        SwingUtilities.invokeLater(() -> {
            LauncherUI launcher = new LauncherUI();
            launcher.setVisible(true);
        });
    }

    private void onAbrirEstatisticas() {
        EstatisticasUI estatisticas = new EstatisticasUI(motor);
        estatisticas.setVisible(true);
    }

    private JPanel createSliderPanel(String title, JSlider slider) {
        JPanel panel = new JPanel(new BorderLayout());
        panel.setBackground(UITheme.BG);
        JLabel lbl = new JLabel(title);
        UITheme.styleLabel(lbl);
        panel.add(lbl, BorderLayout.NORTH);
        panel.add(slider, BorderLayout.CENTER);
        return panel;
    }

    private void onSalvarLabirinto() {
        JFileChooser chooser = new JFileChooser();
        chooser.setDialogTitle("Salvar labirinto e estatisticas");
        chooser.setSelectedFile(new File("labirinto.txt"));
        int escolha = chooser.showSaveDialog(this);

        if (escolha == JFileChooser.APPROVE_OPTION) {
            File arquivo = chooser.getSelectedFile();
            motor.saveMaze(arquivo.getAbsolutePath());

            JOptionPane.showMessageDialog(this,
                    "Labirinto e estatisticas de TODOS os algoritmos (BFS, DFS, A*,\n"
                            + "Dijkstra e Busca Gulosa) foram salvos em:\n" + arquivo.getAbsolutePath()
                            + "\n\nNao e preciso rodar cada algoritmo manualmente antes de\n"
                            + "salvar: o simulador ja calcula e grava as estatisticas de\n"
                            + "todos eles automaticamente para este labirinto.",
                    "Salvo com sucesso",
                    JOptionPane.INFORMATION_MESSAGE);
        }
    }
}