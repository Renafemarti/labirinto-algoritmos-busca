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
        setSize(360, 460);
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

        JPanel botoes = new JPanel(new GridLayout(1, 3, 4, 4));
        JButton bfsBtn = new JButton("BFS");
        JButton dfsBtn = new JButton("DFS");
        JButton astarBtn = new JButton("A*");

        bfsBtn.addActionListener(e -> motor.setAlgorithm(0));
        dfsBtn.addActionListener(e -> motor.setAlgorithm(1));
        astarBtn.addActionListener(e -> motor.setAlgorithm(2));

        botoes.add(bfsBtn);
        botoes.add(dfsBtn);
        botoes.add(astarBtn);

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
        JPanel panel = new JPanel(new GridLayout(5, 1, 4, 4));

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

        panel.add(novoLabirintoBtn);
        panel.add(reiniciarBtn);
        panel.add(voltarBtn);
        panel.add(pausarBtn);
        panel.add(salvarBtn);
        
        return panel;
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
                    "Labirinto e estatisticas salvos em:\n" + arquivo.getAbsolutePath()
                            + "\n\nDica: rode outro algoritmo (BFS/DFS/A*) e salve de novo\n"
                            + "no MESMO arquivo para acumular as estatisticas de todos\n"
                            + "para comparacao posterior.",
                    "Salvo com sucesso",
                    JOptionPane.INFORMATION_MESSAGE);
        }
    }
}