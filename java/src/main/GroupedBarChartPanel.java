package main;

import javax.swing.JPanel;
import java.awt.BasicStroke;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.FontMetrics;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.RenderingHints;

/**
 * Painel que desenha um gráfico de barras AGRUPADAS (várias métricas lado a
 * lado, dentro de cada grupo/algoritmo), no estilo dos gráficos gerados pelo
 * matplotlib: título centralizado, legenda no topo, eixo Y com escala
 * automática e rótulos numéricos acima de cada barra.
 *
 * Desenhado inteiramente com Graphics2D puro (sem bibliotecas externas de
 * gráficos), para não adicionar dependências ao projeto.
 */
public class GroupedBarChartPanel extends JPanel {

    private static final long serialVersionUID = 1L;

    private String title = "";
    private String yAxisLabel = "";
    private String[] groupLabels = new String[0];   // ex.: BFS, DFS, A*, ...
    private String[] seriesLabels = new String[0];   // ex.: Custo, Nos Expandidos, ...
    private Color[] seriesColors = new Color[0];
    private double[][] values = new double[0][0];    // values[grupo][serie]

    public GroupedBarChartPanel() {
        setBackground(UITheme.BG);
        setPreferredSize(new Dimension(900, 620));
    }

    public void setData(String title, String yAxisLabel,
                         String[] groupLabels, String[] seriesLabels, Color[] seriesColors,
                         double[][] values) {
        this.title = title;
        this.yAxisLabel = yAxisLabel;
        this.groupLabels = groupLabels;
        this.seriesLabels = seriesLabels;
        this.seriesColors = seriesColors;
        this.values = values;
        repaint();
    }

    @Override
    protected void paintComponent(Graphics g) {
        super.paintComponent(g);
        Graphics2D g2 = (Graphics2D) g;
        g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
        g2.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, RenderingHints.VALUE_TEXT_ANTIALIAS_ON);

        int panelWidth = getWidth();
        int panelHeight = getHeight();

        // ---------- Título ----------
        g2.setColor(UITheme.TEXT_PRIMARY);
        g2.setFont(UITheme.FONT_TITLE);
        FontMetrics titleMetrics = g2.getFontMetrics();
        int titleX = (panelWidth - titleMetrics.stringWidth(title)) / 2;
        g2.drawString(title, Math.max(titleX, 10), 26);

        if (values.length == 0 || seriesLabels.length == 0) {
            g2.setColor(UITheme.TEXT_SECONDARY);
            g2.setFont(UITheme.FONT_BASE);
            g2.drawString("Sem dados para exibir.", 20, 60);
            return;
        }

        // ---------- Área do gráfico (caixa com eixos) ----------
        int marginLeft = 60;
        int marginRight = 30;
        int marginTop = 90;   // espaço para título + legenda
        int marginBottom = 55;

        int plotLeft = marginLeft;
        int plotRight = panelWidth - marginRight;
        int plotTop = marginTop;
        int plotBottom = panelHeight - marginBottom;
        int plotHeight = plotBottom - plotTop;
        int plotWidth = plotRight - plotLeft;

        // ---------- Legenda (uma linha, no topo da área do gráfico) ----------
        drawLegend(g2, plotLeft, 46);

        // ---------- Escala do eixo Y ----------
        double maxValue = 1;
        for (double[] group : values) {
            for (double v : group) {
                maxValue = Math.max(maxValue, v);
            }
        }
        double niceMax = niceCeiling(maxValue);
        int tickCount = 7; // 0..niceMax em 7 divisões, como na imagem de referência
        double tickStep = niceMax / tickCount;

        g2.setFont(UITheme.FONT_SMALL);
        FontMetrics tickMetrics = g2.getFontMetrics();

        g2.setColor(UITheme.TEXT_SECONDARY);
        for (int i = 0; i <= tickCount; i++) {
            double tickValue = tickStep * i;
            int y = plotBottom - (int) Math.round((tickValue / niceMax) * plotHeight);

            g2.setColor(UITheme.BORDER);
            g2.drawLine(plotLeft, y, plotRight, y);

            g2.setColor(UITheme.TEXT_SECONDARY);
            String label = formatValue(tickValue);
            int labelWidth = tickMetrics.stringWidth(label);
            g2.drawString(label, plotLeft - labelWidth - 8, y + 4);
        }

        // Caixa do gráfico (bordas)
        g2.setColor(UITheme.BORDER_LIGHT);
        g2.setStroke(new BasicStroke(1.2f));
        g2.drawRect(plotLeft, plotTop, plotWidth, plotHeight);

        // ---------- Rótulo do eixo Y (texto vertical) ----------
        g2.setColor(UITheme.TEXT_PRIMARY);
        g2.setFont(UITheme.FONT_BASE);
        Graphics2D gRot = (Graphics2D) g2.create();
        gRot.rotate(-Math.PI / 2);
        FontMetrics yLabelMetrics = gRot.getFontMetrics();
        int yLabelX = -(plotTop + plotHeight / 2) - yLabelMetrics.stringWidth(yAxisLabel) / 2;
        gRot.drawString(yAxisLabel, yLabelX, 18);
        gRot.dispose();

        // ---------- Barras agrupadas ----------
        int groupCount = groupLabels.length;
        int seriesCount = seriesLabels.length;
        int groupSlot = plotWidth / Math.max(groupCount, 1);

        double groupPaddingFraction = 0.18; // espaço vazio entre grupos
        int usableGroupWidth = (int) (groupSlot * (1.0 - groupPaddingFraction));
        int barWidth = usableGroupWidth / Math.max(seriesCount, 1);

        g2.setFont(UITheme.FONT_SMALL);
        FontMetrics valueMetrics = g2.getFontMetrics();

        for (int gI = 0; gI < groupCount; gI++) {
            int groupStartX = plotLeft + groupSlot * gI + (groupSlot - usableGroupWidth) / 2;

            double[] groupValues = gI < values.length ? values[gI] : new double[seriesCount];

            for (int sI = 0; sI < seriesCount; sI++) {
                double value = sI < groupValues.length ? groupValues[sI] : 0;
                int barHeight = (int) Math.round((value / niceMax) * plotHeight);
                if (value > 0 && barHeight < 2) barHeight = 2;

                int barX = groupStartX + barWidth * sI;
                int barY = plotBottom - barHeight;

                Color color = seriesColors[sI % seriesColors.length];
                g2.setColor(color);
                g2.fillRect(barX, barY, Math.max(barWidth - 2, 1), barHeight);

                // Valor acima da barra
                String valueText = formatValue(value);
                g2.setColor(UITheme.TEXT_PRIMARY);
                int valueX = barX + (barWidth - valueMetrics.stringWidth(valueText)) / 2;
                g2.drawString(valueText, valueX, barY - 6);
            }

            // Rótulo do grupo (nome do algoritmo), centralizado sob o grupo
            String groupLabel = groupLabels[gI];
            int labelWidth = valueMetrics.stringWidth(groupLabel);
            int labelX = plotLeft + groupSlot * gI + (groupSlot - labelWidth) / 2;
            g2.setColor(UITheme.TEXT_PRIMARY);
            g2.drawString(groupLabel, labelX, plotBottom + 20);
        }
    }

    private void drawLegend(Graphics2D g2, int startX, int startY) {
        g2.setFont(UITheme.FONT_SMALL);
        FontMetrics fm = g2.getFontMetrics();

        int swatchSize = 12;
        int gapAfterSwatch = 6;
        int gapAfterEntry = 22;

        int x = startX;
        int y = startY;
        for (int i = 0; i < seriesLabels.length; i++) {
            Color color = seriesColors[i % seriesColors.length];
            g2.setColor(color);
            g2.fillRoundRect(x, y - swatchSize + 2, swatchSize, swatchSize, 4, 4);

            g2.setColor(UITheme.TEXT_PRIMARY);
            g2.drawString(seriesLabels[i], x + swatchSize + gapAfterSwatch, y + 2);

            x += swatchSize + gapAfterSwatch + fm.stringWidth(seriesLabels[i]) + gapAfterEntry;
        }
    }

    private String formatValue(double value) {
        if (value == Math.rint(value)) {
            return String.valueOf((long) value);
        }
        return String.format("%.1f", value);
    }

    // Arredonda 'value' para cima, para um numero "redondo" (1/2/5 x 10^n),
    // igual ao que o matplotlib costuma escolher automaticamente para o
    // topo do eixo Y (ex.: 700 em vez de 617).
    private double niceCeiling(double value) {
        if (value <= 0) return 1;
        double exponent = Math.floor(Math.log10(value));
        double magnitude = Math.pow(10, exponent);
        double fraction = value / magnitude;

        double niceFraction;
        if (fraction <= 1) niceFraction = 1;
        else if (fraction <= 2) niceFraction = 2;
        else if (fraction <= 5) niceFraction = 5;
        else if (fraction <= 7) niceFraction = 7;
        else niceFraction = 10;

        return niceFraction * magnitude;
    }
}