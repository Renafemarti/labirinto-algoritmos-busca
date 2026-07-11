package main;

import javax.swing.SwingUtilities;

public class Main {

    public static void main(String[] args) {
        // A aplicacao comeca pela tela de lancamento (LauncherUI), onde o
        // usuario escolhe o tamanho do labirinto (ou abre um arquivo salvo).
        // So depois de clicar em "Gerar Labirinto" e que a janela OpenGL e
        // os controles (TransformUI) sao criados.
        SwingUtilities.invokeLater(() -> {
            LauncherUI launcher = new LauncherUI();
            launcher.setVisible(true);
        });
    }
}