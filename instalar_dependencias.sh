#!/bin/bash
# ============================================================
# Instalador de dependências - Labirinto (Algoritmos de Busca)
# Debian / Ubuntu / WSL (Ubuntu ou Debian)
# ============================================================
set -e

echo -e "\e[1;36m==> Atualizando lista de pacotes...\e[0m"
sudo apt update

echo -e "\e[1;36m==> Instalando toolchain e dependências gráficas (C++/OpenGL/GLFW/GLM)...\e[0m"
sudo apt install -y \
    build-essential \
    libglfw3-dev \
    libgl1-mesa-dev \
    xorg-dev \
    libglm-dev

echo -e "\e[1;36m==> Instalando JDK 21 completo (necessário: jawt.h/libjawt.so + javac)...\e[0m"
# IMPORTANTE: precisa ser o pacote "jdk" completo, NÃO o "-headless".
# O jawt.h/libjawt.so (usado para embutir a janela GLFW dentro do Canvas
# Swing) só vem no pacote completo. Também é o pacote que cria o caminho
# /usr/lib/jvm/java-21-openjdk-amd64, usado (fixo) dentro do rodar.sh.
sudo apt install -y openjdk-21-jdk

echo -e "\e[1;36m==> Instalando plugin de decoração de janela (Wayland/libdecor)...\e[0m"
# Opcional: evita um aviso/falha de decoração da janela GLFW em desktops
# Wayland modernos. Pode não existir em algumas distros Debian; se faltar,
# o projeto ainda funciona normalmente (ambiente X11/WSL não precisa dele).
sudo apt install -y libdecor-0-plugin-1-gtk || echo -e "\e[1;33mAviso: libdecor-0-plugin-1-gtk indisponível neste repositório, ignorando (não é crítico).\e[0m"

echo ""
echo -e "\e[1;32m==> Tudo instalado! Verificação rápida:\e[0m"
g++ --version | head -n1
javac --version
pkg-config --modversion glfw3 2>/dev/null && echo "GLFW OK"
[ -f /usr/lib/jvm/java-21-openjdk-amd64/include/jawt.h ] && echo "jawt.h OK" || echo -e "\e[1;31mjawt.h NÃO encontrado — algo deu errado.\e[0m"

echo ""
echo -e "\e[1;32mPronto! Agora rode:\e[0m"
echo "    chmod +x rodar.sh"
echo "    ./rodar.sh"
