package main;

import javax.swing.*;
import java.awt.*;
import java.io.File;

public class LauncherUI extends JFrame {

    private static final long serialVersionUID = 1L;

    private static final int MIN_SIZE = 5;
    private static final int MAX_SIZE = 100;
    private static final int DEFAULT_WIDTH = 25;
    private static final int DEFAULT_HEIGHT = 20;

    // Opcoes exibidas no combo, na mesma ordem dos ids esperados por
    // MotorGrafico.setMazeAlgorithm() (0, 1, 2 - ver motor.cpp).
    private static final String[] ALGORITMOS_GERACAO = {
            "Recursive Backtracking", "Prim", "Kruskal"
    };

    private JSpinner spinnerLargura;
    private JSpinner spinnerAltura;
    private JComboBox<String> comboAlgoritmoGeracao;

    public LauncherUI() {
        setTitle("Simulador de Labirinto");
        setSize(600, 450);
        setLocationRelativeTo(null);
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setResizable(false);

        JPanel content = new JPanel(new BorderLayout(10, 10));
        content.setBorder(BorderFactory.createEmptyBorder(16, 16, 16, 16));
        content.setBackground(UITheme.BG);
        setContentPane(content);

        JPanel centro = new JPanel(new BorderLayout(10, 10));
        centro.setBackground(UITheme.BG);
        centro.add(createCamposTamanho(), BorderLayout.NORTH);
        centro.add(createCampoAlgoritmoGeracao(), BorderLayout.SOUTH);

        content.add(createTitulo(), BorderLayout.NORTH);
        content.add(centro, BorderLayout.CENTER);
        content.add(createBotoes(), BorderLayout.SOUTH);
    }

    private JLabel createTitulo() {
        JLabel titulo = new JLabel("Configuracoes do Labirinto", SwingConstants.CENTER);
        titulo.setFont(UITheme.FONT_TITLE);
        titulo.setForeground(UITheme.TEXT_PRIMARY);
        return titulo;
    }

    private JPanel createCamposTamanho() {
        JPanel panel = new JPanel(new GridLayout(2, 2, 8, 12));
        panel.setBackground(UITheme.BG);
        panel.setBorder(UITheme.titledBorder("Tamanho (em blocos)"));

        spinnerLargura = new JSpinner(
                new SpinnerNumberModel(DEFAULT_WIDTH, MIN_SIZE, MAX_SIZE, 1));
        spinnerAltura = new JSpinner(
                new SpinnerNumberModel(DEFAULT_HEIGHT, MIN_SIZE, MAX_SIZE, 1));
        UITheme.styleSpinner(spinnerLargura);
        UITheme.styleSpinner(spinnerAltura);

        JLabel lblLargura = new JLabel("Largura:");
        JLabel lblAltura = new JLabel("Altura:");
        UITheme.styleLabel(lblLargura);
        UITheme.styleLabel(lblAltura);

        panel.add(lblLargura);
        panel.add(spinnerLargura);
        panel.add(lblAltura);
        panel.add(spinnerAltura);

        return panel;
    }

    // Combo para escolher qual algoritmo de GERACAO sera usado no
    // labirinto criado por "Gerar Labirinto" (nao se aplica a "Abrir
    // Labirinto Salvo...", que so carrega o que ja esta no arquivo).
    private JPanel createCampoAlgoritmoGeracao() {
        JPanel panel = new JPanel(new BorderLayout(4, 4));
        panel.setBackground(UITheme.BG);
        panel.setBorder(UITheme.titledBorder("Algoritmo de Geracao"));

        comboAlgoritmoGeracao = new JComboBox<>(ALGORITMOS_GERACAO);
        comboAlgoritmoGeracao.setSelectedIndex(0);

        panel.add(comboAlgoritmoGeracao, BorderLayout.CENTER);
        return panel;
    }

    private JPanel createBotoes() {
        JPanel panel = new JPanel(new GridLayout(2, 1, 0, 8));
        panel.setBackground(UITheme.BG);

        JButton abrirBtn = UITheme.button("Abrir Labirinto Salvo...");
        abrirBtn.addActionListener(e -> onAbrirArquivo());

        JButton gerarBtn = UITheme.button("Gerar Labirinto");
        gerarBtn.addActionListener(e -> onGerarLabirinto());

        panel.add(abrirBtn);
        panel.add(gerarBtn);
        return panel;
    }

    private void onAbrirArquivo() {
        JFileChooser chooser = new JFileChooser();
        chooser.setDialogTitle("Selecione um labirinto salvo");
        int escolha = chooser.showOpenDialog(this);

        if (escolha != JFileChooser.APPROVE_OPTION) return;

        File arquivo = chooser.getSelectedFile();
        String caminho = arquivo.getAbsolutePath();

        MotorGrafico motor = new MotorGrafico();

        boolean valido = motor.validateMazeFile(caminho);
        if (!valido) {
            JOptionPane.showMessageDialog(this,
                    "O arquivo selecionado nao esta no formato correto de labirinto salvo:\n"
                            + caminho
                            + "\n\nCertifique-se de escolher um arquivo gerado pelo botao\n"
                            + "\"Salvar Labirinto e Estatisticas\" da tela de controles.",
                    "Arquivo invalido",
                    JOptionPane.ERROR_MESSAGE);
            return;
        }

        abrirControlesEIniciarMotor(motor, ui -> motor.initFromFile(ui.getMazeCanvas(), caminho));
    }

    private void onGerarLabirinto() {
        int largura = (Integer) spinnerLargura.getValue();
        int altura = (Integer) spinnerAltura.getValue();

        MotorGrafico motor = new MotorGrafico();
        motor.setMazeAlgorithm(comboAlgoritmoGeracao.getSelectedIndex());

        abrirControlesEIniciarMotor(motor, ui -> motor.init(ui.getMazeCanvas(), largura, altura));
    }

    /**
     * Cria e mostra a TransformUI (janela unica: controles + labirinto) e,
     * SOMENTE DEPOIS que ela estiver visivel (o Canvas precisa estar
     * "displayable" para o JAWT conseguir pegar a Window X11 por tras
     * dele), inicia a thread OpenGL chamando 'iniciarMotor'.
     *
     * Isso substitui o fluxo antigo, em que a thread OpenGL era iniciada
     * imediatamente e criava sua propria janela GLFW independente — agora
     * a janela GLFW nasce reparentada dentro do Canvas, entao a ordem
     * importa: primeiro a janela Swing aparece, depois o motor arranca.
     */
    private void abrirControlesEIniciarMotor(MotorGrafico motor, java.util.function.Consumer<TransformUI> iniciarMotor) {
        SwingUtilities.invokeLater(() -> {
            TransformUI ui = new TransformUI(motor);
            ui.setVisible(true);

            Thread glThread = new Thread(() -> iniciarMotor.accept(ui), "OpenGL-Thread");
            glThread.setDaemon(true);
            glThread.start();

            // Garante que o primeiro tamanho real do Canvas seja mandado
            // ao lado nativo (componentResized pode nao disparar se o
            // tamanho inicial do layout ja "nasceu" correto).
            SwingUtilities.invokeLater(ui::syncInitialCanvasSize);
        });
        dispose(); // fecha a tela de lancamento
    }
}