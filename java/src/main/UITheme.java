package main;

import javax.swing.*;
import javax.swing.border.Border;
import javax.swing.border.CompoundBorder;
import javax.swing.border.EmptyBorder;
import javax.swing.border.TitledBorder;
import javax.swing.plaf.basic.BasicSpinnerUI;
import javax.swing.plaf.basic.BasicSliderUI;
import java.awt.*;
import java.util.Set;
import java.util.HashSet;

/**
 * Paleta de cores e fonte unica usada em TODAS as telas Swing do projeto
 * (LauncherUI, TransformUI, EstatisticasUI, GroupedBarChartPanel), para que
 * o simulador inteiro tenha uma identidade visual consistente: fundo azul
 * marinho bem escuro, painel/"balao" um pouco mais claro, azul-acinzentado
 * como cor de destaque (botoes, icones, bordas) e texto quase branco.
 *
 * Paleta inspirada na janela de erro de referencia (fundo escuro, balao de
 * fala azul acinzentado, botao "OK" arredondado na mesma familia de azul).
 *
 * Uso tipico numa tela:
 *   UITheme.applyGlobalDefaults();      // 1x, antes de criar qualquer JFrame
 *   frame.getContentPane().setBackground(UITheme.BG);
 *   panel.setBorder(UITheme.titledBorder("Titulo"));
 *   JButton b = UITheme.button("Gerar Labirinto");
 */
public final class UITheme {

    private UITheme() {}

    // ---------------------------------------------------------------
    // Paleta de cores
    // ---------------------------------------------------------------
    public static final Color BG            = new Color(0x1B1E2C); // fundo principal das janelas
    public static final Color BG_PANEL      = new Color(0x232740); // paineis/caixas internas
    public static final Color BG_ELEVATED   = new Color(0x2B3050); // campos de entrada, areas "elevadas"

    public static final Color ACCENT        = new Color(0x5B79A8); // azul de destaque (icones, graficos)
    public static final Color ACCENT_SOFT   = new Color(0x7C8CB4); // botoes / balao de fala
    public static final Color ACCENT_HOVER  = new Color(0x93A2C6); // hover dos botoes
    public static final Color ACCENT_PRESS  = new Color(0x64729B); // botao pressionado

    public static final Color BORDER        = new Color(0x3C4160);
    public static final Color BORDER_LIGHT  = new Color(0x565C82);

    public static final Color TEXT_PRIMARY   = new Color(0xEDEFF7);
    public static final Color TEXT_SECONDARY = new Color(0xAAB2CE);
    public static final Color TEXT_ON_ACCENT = new Color(0xF5F7FC);

    // ---------------------------------------------------------------
    // Fonte: usa a melhor fonte de sistema disponivel (Segoe UI no
    // Windows, Inter/Ubuntu no Linux, SF Pro/Helvetica no macOS) com
    // fallback em cascata ate a SansSerif logica do Java.
    // ---------------------------------------------------------------
    private static final String[] FONT_CANDIDATES = {
            "Segoe UI Variable", "Segoe UI", "Inter", "Ubuntu", "Noto Sans",
            "SF Pro Text", "Helvetica Neue", "Roboto"
    };

    private static final String FAMILY = resolveFamily();

    public static final Font FONT_BASE  = new Font(FAMILY, Font.PLAIN, 13);
    public static final Font FONT_BOLD  = new Font(FAMILY, Font.BOLD, 13);
    public static final Font FONT_TITLE = new Font(FAMILY, Font.BOLD, 17);
    public static final Font FONT_SMALL = new Font(FAMILY, Font.PLAIN, 12);

    private static String resolveFamily() {
        Set<String> disponiveis = new HashSet<>();
        for (String f : GraphicsEnvironment.getLocalGraphicsEnvironment().getAvailableFontFamilyNames()) {
            disponiveis.add(f);
        }
        for (String candidata : FONT_CANDIDATES) {
            if (disponiveis.contains(candidata)) return candidata;
        }
        return Font.SANS_SERIF;
    }

    // ---------------------------------------------------------------
    // Aplica os defaults do UIManager. Chame UMA VEZ, bem no inicio do
    // main() (antes de qualquer componente Swing ser criado), para que
    // dialogos padrao (JOptionPane, JFileChooser) e componentes que nao
    // tem cor setada explicitamente ja nasçam com a paleta correta.
    // ---------------------------------------------------------------
    public static void applyGlobalDefaults() {
        UIManager.put("control", BG);
        UIManager.put("info", BG_PANEL);
        UIManager.put("nimbusBase", ACCENT);
        UIManager.put("text", TEXT_PRIMARY);

        put("Panel.background", BG);
        put("Panel.foreground", TEXT_PRIMARY);
        put("OptionPane.background", BG_PANEL);
        put("OptionPane.messageForeground", TEXT_PRIMARY);
        put("OptionPane.buttonFont", FONT_BASE);
        put("OptionPane.messageFont", FONT_BASE);

        put("Label.foreground", TEXT_PRIMARY);
        put("Label.font", FONT_BASE);

        put("Button.background", ACCENT_SOFT);
        put("Button.foreground", TEXT_ON_ACCENT);
        put("Button.font", FONT_BOLD);
        put("Button.select", ACCENT_PRESS);
        put("Button.focus", new Color(0, 0, 0, 0));

        put("TextField.background", BG_ELEVATED);
        put("TextField.foreground", TEXT_PRIMARY);
        put("TextField.caretForeground", TEXT_PRIMARY);
        put("TextField.font", FONT_BASE);

        put("TextArea.background", BG);
        put("TextArea.foreground", TEXT_SECONDARY);
        put("TextArea.font", FONT_BASE);

        put("Spinner.background", BG_ELEVATED);
        put("Spinner.foreground", TEXT_PRIMARY);
        put("Spinner.font", FONT_BASE);
        put("FormattedTextField.background", BG_ELEVATED);
        put("FormattedTextField.foreground", TEXT_PRIMARY);
        put("FormattedTextField.caretForeground", TEXT_PRIMARY);

        put("ComboBox.background", BG_ELEVATED);
        put("ComboBox.foreground", TEXT_PRIMARY);
        put("ComboBox.font", FONT_BASE);

        put("Slider.foreground", TEXT_PRIMARY);
        put("Slider.font", FONT_BASE);

        put("TitledBorder.titleColor", ACCENT_HOVER);
        put("TitledBorder.font", FONT_BOLD);

        put("FileChooser.background", BG);
        put("FileChooser.foreground", TEXT_PRIMARY);

        put("List.background", BG_ELEVATED);
        put("List.foreground", TEXT_PRIMARY);
        put("List.font", FONT_BASE);

        put("ToolTip.background", BG_ELEVATED);
        put("ToolTip.foreground", TEXT_PRIMARY);
        put("ToolTip.font", FONT_SMALL);
    }

    private static void put(String key, Object value) {
        UIManager.put(key, value);
    }

    // ---------------------------------------------------------------
    // Helpers de componentes prontos, ja no padrao visual
    // ---------------------------------------------------------------

    /** Borda com titulo colorido/fonte do tema, com respiro interno. */
    public static Border titledBorder(String titulo) {
        TitledBorder tb = BorderFactory.createTitledBorder(
                BorderFactory.createLineBorder(BORDER, 1, true), titulo);
        tb.setTitleColor(ACCENT_HOVER);
        tb.setTitleFont(FONT_BOLD);
        return new CompoundBorder(tb, new EmptyBorder(6, 8, 8, 8));
    }

    /** Botao arredondado no padrao azul-acinzentado da imagem de referencia. */
    public static JButton button(String texto) {
        return new RoundedButton(texto);
    }

    /** Aplica fundo/cor de texto padrao a um painel comum. */
    public static void stylePanel(JPanel p) {
        p.setBackground(BG);
        p.setForeground(TEXT_PRIMARY);
        p.setFont(FONT_BASE);
    }

    /** Aplica fundo/cor de texto padrao a um rotulo. */
    public static void styleLabel(JLabel l) {
        l.setForeground(TEXT_PRIMARY);
        l.setFont(FONT_BASE);
    }

    /** Estiliza um JSlider para combinar com a paleta escura. */
    public static void styleSlider(JSlider slider) {
        slider.setBackground(BG);
        slider.setForeground(TEXT_PRIMARY);
        slider.setFont(FONT_SMALL);
        slider.setUI(new BasicSliderUI(slider) {
            @Override
            public void paintTrack(Graphics g) {
                Graphics2D g2 = (Graphics2D) g.create();
                g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
                Rectangle track = trackRect;
                g2.setColor(BG_ELEVATED);
                g2.fillRoundRect(track.x, track.y + track.height / 2 - 3, track.width, 6, 6, 6);
                g2.setColor(ACCENT_SOFT);
                int filled = thumbRect.x + thumbRect.width / 2 - track.x;
                g2.fillRoundRect(track.x, track.y + track.height / 2 - 3, Math.max(filled, 0), 6, 6, 6);
                g2.dispose();
            }

            @Override
            public void paintThumb(Graphics g) {
                Graphics2D g2 = (Graphics2D) g.create();
                g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
                g2.setColor(ACCENT_HOVER);
                g2.fillOval(thumbRect.x, thumbRect.y + thumbRect.height / 2 - 7, 14, 14);
                g2.dispose();
            }

            @Override
            public void paintFocus(Graphics g) { /* sem retangulo de foco */ }
        });
    }

    /** Estiliza um JSpinner (numero de largura/altura do labirinto). */
    public static void styleSpinner(JSpinner spinner) {
        spinner.setFont(FONT_BASE);
        spinner.setBackground(BG_ELEVATED);
        spinner.setBorder(BorderFactory.createLineBorder(BORDER, 1, true));
        spinner.setUI(new BasicSpinnerUI());
        JComponent editor = spinner.getEditor();
        if (editor instanceof JSpinner.DefaultEditor) {
            JTextField field = ((JSpinner.DefaultEditor) editor).getTextField();
            field.setBackground(BG_ELEVATED);
            field.setForeground(TEXT_PRIMARY);
            field.setCaretColor(TEXT_PRIMARY);
            field.setBorder(new EmptyBorder(2, 6, 2, 6));
        }
        for (Component c : spinner.getComponents()) {
            c.setBackground(BG_ELEVATED);
            c.setForeground(TEXT_PRIMARY);
        }
    }

    // ---------------------------------------------------------------
    // Botao arredondado customizado (Graphics2D puro, sem dependencias)
    // ---------------------------------------------------------------
    private static class RoundedButton extends JButton {
        private static final long serialVersionUID = 1L;

        RoundedButton(String text) {
            super(text);
            setFont(FONT_BOLD);
            setForeground(TEXT_ON_ACCENT);
            setContentAreaFilled(false);
            setFocusPainted(false);
            setBorderPainted(false);
            setOpaque(false);
            setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
            setBorder(new EmptyBorder(8, 16, 8, 16));
        }

        @Override
        protected void paintComponent(Graphics g) {
            Graphics2D g2 = (Graphics2D) g.create();
            g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);

            Color fill = ACCENT_SOFT;
            if (getModel().isPressed()) fill = ACCENT_PRESS;
            else if (getModel().isRollover()) fill = ACCENT_HOVER;
            else if (!isEnabled()) fill = BORDER;

            g2.setColor(fill);
            g2.fillRoundRect(0, 0, getWidth() - 1, getHeight() - 1, 14, 14);
            g2.dispose();
            super.paintComponent(g);
        }

        @Override
        protected void paintBorder(Graphics g) {
            Graphics2D g2 = (Graphics2D) g.create();
            g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
            g2.setColor(BORDER_LIGHT);
            g2.drawRoundRect(0, 0, getWidth() - 1, getHeight() - 1, 14, 14);
            g2.dispose();
        }
    }
}
