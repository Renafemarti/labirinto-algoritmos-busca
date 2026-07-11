package main;

import javax.swing.BorderFactory;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.SwingConstants;
import java.awt.BorderLayout;
import java.awt.Color;
import java.util.ArrayList;
import java.util.List;

/**
 * Janela que mostra, em um único gráfico de barras agrupadas, as
 * estatísticas dos algoritmos de busca (BFS, DFS, A*, Dijkstra e Busca
 * Gulosa) para o labirinto atualmente exibido: custo do caminho, nós
 * expandidos, máximo de nós em memória e número de iterações — todos
 * juntos, um grupo de barras por algoritmo, igual ao layout de
 * "Comparação de Desempenho das Buscas no Labirinto".
 *
 * Os dados vêm de {@link MotorGrafico#getStatistics()}, que roda os
 * algoritmos no lado nativo (C++) e devolve o resultado em texto.
 */
public class EstatisticasUI extends JFrame {

    private static final long serialVersionUID = 1L;

    private static final String[] SERIES_LABELS = {
            "Custo", "Nos Expandidos", "Max Nos Memoria", "Interacoes"
    };

    private static final Color[] SERIES_COLORS = {
            new Color(0x1F77B4), // Custo            - azul
            new Color(0xFF7F0E), // Nos Expandidos    - laranja
            new Color(0x2CA02C), // Max Nos Memoria   - verde
            new Color(0xD62728)  // Interacoes        - vermelho
    };

    // Uma linha do texto recebido de getStatistics():
    // nome;custo;nos_expandidos;max_nos_memoria;iteracoes
    private static class Estatistica {
        String nome;
        double custo;
        double nosExpandidos;
        double maxNosMemoria;
        double iteracoes;
    }

    public EstatisticasUI(MotorGrafico motor) {
        setTitle("Comparacao de Desempenho das Buscas no Labirinto");
        setSize(1000, 680);
        setLocationRelativeTo(null);
        setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
        setLayout(new BorderLayout(8, 8));

        List<Estatistica> dados = carregarEstatisticas(motor);

        if (dados.isEmpty()) {
            JPanel vazio = new JPanel(new BorderLayout());
            JLabel aviso = new JLabel(
                    "Nao foi possivel obter as estatisticas do labirinto atual.",
                    SwingConstants.CENTER);
            aviso.setBorder(BorderFactory.createEmptyBorder(24, 24, 24, 24));
            vazio.add(aviso, BorderLayout.CENTER);
            add(vazio, BorderLayout.CENTER);
            return;
        }

        String[] groupLabels = new String[dados.size()];
        double[][] valores = new double[dados.size()][SERIES_LABELS.length];

        for (int i = 0; i < dados.size(); i++) {
            Estatistica e = dados.get(i);
            groupLabels[i] = e.nome;
            valores[i][0] = Math.max(e.custo, 0);
            valores[i][1] = e.nosExpandidos;
            valores[i][2] = e.maxNosMemoria;
            valores[i][3] = e.iteracoes;
        }

        GroupedBarChartPanel grafico = new GroupedBarChartPanel();
        grafico.setData(
                "Comparacao de Desempenho das Buscas no Labirinto",
                "Variaveis de Desempenho",
                groupLabels,
                SERIES_LABELS,
                SERIES_COLORS,
                valores);

        add(grafico, BorderLayout.CENTER);
    }

    private List<Estatistica> carregarEstatisticas(MotorGrafico motor) {
        List<Estatistica> lista = new ArrayList<>();

        String texto;
        try {
            texto = motor.getStatistics();
        } catch (Exception ex) {
            JOptionPane.showMessageDialog(this,
                    "Erro ao consultar as estatisticas do labirinto:\n" + ex.getMessage(),
                    "Erro", JOptionPane.ERROR_MESSAGE);
            return lista;
        }

        if (texto == null || texto.isBlank()) {
            return lista;
        }

        for (String linha : texto.split("\n")) {
            linha = linha.trim();
            if (linha.isEmpty()) continue;

            String[] campos = linha.split(";");
            if (campos.length != 5) continue; // linha em formato inesperado, ignora

            try {
                Estatistica e = new Estatistica();
                e.nome = campos[0];
                e.custo = Double.parseDouble(campos[1]);
                e.nosExpandidos = Double.parseDouble(campos[2]);
                e.maxNosMemoria = Double.parseDouble(campos[3]);
                e.iteracoes = Double.parseDouble(campos[4]);
                lista.add(e);
            } catch (NumberFormatException ex) {
                // linha corrompida/inesperada: ignora e continua com as demais
            }
        }

        return lista;
    }
}