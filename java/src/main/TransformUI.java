package main;

import javax.swing.*;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;
import java.awt.*;
import java.io.File;

public class TransformUI extends JFrame {

    private static final long serialVersionUID = 1L;

    private final MotorGrafico motor;

    private JSlider sliderYaw;
    private JSlider sliderPitch;
    private JSlider sliderSpeed;

    public TransformUI(MotorGrafico motor) {
        this.motor = motor;

        setTitle("Simulador de Labirinto - Controles");
        setSize(600, 460);
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setLayout(new BorderLayout(8, 8));

        add(createAlgoritmoPanel(), BorderLayout.NORTH);
        add(createCameraPanel(), BorderLayout.CENTER);
        add(createAcoesPanel(), BorderLayout.SOUTH);

        // Ao fechar a janela Swing, libera os recursos do OpenGL também.
        addWindowListener(new java.awt.event.WindowAdapter() {
            @Override
            public void windowClosing(java.awt.event.WindowEvent e) {
                motor.cleanup();
            }
        });
    }

private JPanel createAlgoritmoPanel() {
        JPanel panel = new JPanel(new GridLayout(2, 1));
        panel.setBorder(BorderFactory.createTitledBorder("Algoritmo de Busca"));

        JPanel botoes = new JPanel(new GridLayout(1, 5, 4, 4));
        JButton bfsBtn = new JButton("BFS");
        JButton dfsBtn = new JButton("DFS");
        JButton astarBtn = new JButton("A*");
        JButton dijkstraBtn = new JButton("Dijkstra");
        JButton gulosaBtn = new JButton("Gulosa (GBS)");

        bfsBtn.addActionListener(e -> motor.setAlgorithm(0));
        dfsBtn.addActionListener(e -> motor.setAlgorithm(1));
        astarBtn.addActionListener(e -> motor.setAlgorithm(2));
        dijkstraBtn.addActionListener(e -> motor.setAlgorithm(3));
        gulosaBtn.addActionListener(e -> motor.setAlgorithm(4));

        botoes.add(bfsBtn);
        botoes.add(dfsBtn);
        botoes.add(astarBtn);
        botoes.add(dijkstraBtn);
        botoes.add(gulosaBtn);

        panel.add(new JLabel("Escolha o algoritmo:"));
        panel.add(botoes);
        return panel;
    }

    private JPanel createCameraPanel() {
        JPanel panel = new JPanel(new GridLayout(3, 1, 4, 4));
        panel.setBorder(BorderFactory.createTitledBorder("Camera e Velocidade"));

        sliderYaw = new JSlider(0, 360, 45);
        sliderPitch = new JSlider(1, 89, 45);
        sliderSpeed = new JSlider(1, 200, 20); // representa milissegundos (1 a 200ms)

        ChangeListener cameraListener = (ChangeEvent e) -> updateCamera();
        sliderYaw.addChangeListener(cameraListener);
        sliderPitch.addChangeListener(cameraListener);

        sliderSpeed.addChangeListener((ChangeEvent e) -> {
            float seconds = sliderSpeed.getValue() / 1000.0f;
            motor.setAnimationSpeed(seconds);
        });

        panel.add(createSliderPanel("Rotacao", sliderYaw));
        panel.add(createSliderPanel("Inclinacao", sliderPitch));
        panel.add(createSliderPanel("Velocidade da Animacao", sliderSpeed));
        return panel;
    }

private JPanel createAcoesPanel() {
        JPanel panel = new JPanel(new GridLayout(6, 1, 4, 4));

        JButton novoLabirintoBtn = new JButton("Novo Labirinto");
        novoLabirintoBtn.addActionListener(e -> motor.generateNewMaze());

        JButton reiniciarBtn = new JButton("Reiniciar Animacao");
        reiniciarBtn.addActionListener(e -> motor.resetAnimation());

        JButton voltarBtn = new JButton("Voltar Passo");
        voltarBtn.addActionListener(e -> motor.stepBack());

        JButton pausarBtn = new JButton("Pausar");
        pausarBtn.addActionListener(e -> {
            boolean pausar = pausarBtn.getText().equals("Pausar");
            motor.setPaused(pausar);
            pausarBtn.setText(pausar ? "Continuar" : "Pausar");
        });

        JButton salvarBtn = new JButton("Salvar Labirinto e Estatisticas");
        salvarBtn.addActionListener(e -> onSalvarLabirinto());

        JButton estatisticasBtn = new JButton("Estatisticas");
        estatisticasBtn.addActionListener(e -> onAbrirEstatisticas());

        panel.add(novoLabirintoBtn);
        panel.add(reiniciarBtn);
        panel.add(voltarBtn);
        panel.add(pausarBtn);
        panel.add(salvarBtn);
        panel.add(estatisticasBtn);
        
        return panel;
    }

    private void onAbrirEstatisticas() {
        EstatisticasUI estatisticas = new EstatisticasUI(motor);
        estatisticas.setVisible(true);
    }

    private JPanel createSliderPanel(String title, JSlider slider) {
        JPanel panel = new JPanel(new BorderLayout());
        panel.add(new JLabel(title), BorderLayout.NORTH);
        panel.add(slider, BorderLayout.CENTER);
        return panel;
    }

    private void updateCamera() {
        motor.setCamera(sliderYaw.getValue(), sliderPitch.getValue());
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